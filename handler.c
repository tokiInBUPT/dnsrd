#include "handler.h"
#include "cli.h"
#include "config.h"
#include "idTransfer.h"
#include <WS2tcpip.h>
#include <WinSock2.h>
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
/*
 * 初始化Socket
 */
void initSocket(DNSRD_RUNTIME *runtime) {
    /* 初始化WinSock 2.2  */
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    /* 监听socket  */
    runtime->server = socket(AF_INET, SOCK_DGRAM, 0);
    runtime->listenAddr.sin_family = AF_INET;
    runtime->listenAddr.sin_addr.s_addr = INADDR_ANY;
    runtime->listenAddr.sin_port = htons(runtime->config.port);
    if (bind(runtime->server, (struct sockaddr *)&runtime->listenAddr, sizeof(runtime->listenAddr)) < 0) {
        printf("ERROR: bind faild: %d\n", errno);
        exit(-1);
    }
    /* 上游socket  */
    runtime->client = socket(AF_INET, SOCK_DGRAM, 0);
    runtime->upstreamAddr.sin_family = AF_INET;
    inet_pton(AF_INET, runtime->config.upstream, &runtime->upstreamAddr.sin_addr);
    uint16_t upstreamPort = 53;
    runtime->upstreamAddr.sin_port = htons(upstreamPort);
    /* Winsock UDP connection reset fix */
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(runtime->client, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
    WSAIoctl(runtime->server, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
}
/*
 * 通用接收函数
 */
DNSPacket recvDNSPacket(DNSRD_RUNTIME *runtime, SOCKET socket, Buffer *buffer, struct sockaddr_in *clientAddr, int *error) {
    int clientAddrLength = sizeof(*clientAddr);
    int ret = recvfrom(socket, (char *)buffer->data, buffer->length, 0, (struct sockaddr *)clientAddr, &clientAddrLength);
    if (ret < 0) { //收取失败
        printf("Error: recvfrom %d\n", WSAGetLastError());
        *error = -1; //给函数调用者的标识
        DNSPacket packet;
        packet.answers = NULL;
        packet.questions = NULL;
        packet.additional = NULL;
        packet.authorities = NULL;
        packet.header.questionCount = 0;
        packet.header.answerCount = 0;
        packet.header.additionalCount = 0;
        packet.header.authorityCount = 0;
        return packet;     //返回空包
    } else if (ret == 0) { //等待协议时网络中断
        printf("Error: recvfrom 0\n");
        *error = 0; //给函数调用者的标识
        DNSPacket packet;
        packet.answers = NULL;
        packet.questions = NULL;
        packet.additional = NULL;
        packet.authorities = NULL;
        packet.header.questionCount = 0;
        packet.header.answerCount = 0;
        packet.header.additionalCount = 0;
        packet.header.authorityCount = 0;
        return packet; //返回空包
    }
    buffer->length = ret;                         //接收成功
    *error = ret;                                 //给函数调用者的标识
    DNSPacket packet = DNSPacket_decode(*buffer); //将buffer的数据转成packet
    if (runtime->config.debug) {                  //debug信息
        if (socket == runtime->server) {
            char clientIp[16];
            inet_ntop(AF_INET, &clientAddr->sin_addr, clientIp, sizeof(clientIp));
            printf("C>> Received packet from client %s:%d\n", clientIp, ntohs(clientAddr->sin_port));
        } else {
            printf("S>> Received packet from upstream\n");
        }
        DNSPacket_print(&packet);
    }
    return packet;
}
/*
 * 判断是否可以缓存
 */
int checkCacheable(DNSQType type) {
    if (type == A || type == AAAA || type == CNAME || type == PTR || type == NS || type == TXT) {
        return 1;
    }
    return 0;
}
/*
 * 接收客户端的查询请求
 */
void recvFromClient(DNSRD_RUNTIME *runtime) {
    Buffer buffer = makeBuffer(DNS_PACKET_SIZE); //创建buffer （一段内存空间）
    struct sockaddr_in clientAddr;
    int status = 0; //指示DNS包接收状态
    DNSPacket packet = recvDNSPacket(runtime, runtime->server, &buffer, &clientAddr, &status);
    if (status <= 0) {
        // 接收失败 - 空包，甚至不需要destroy。
        free(buffer.data);
        return;
    }
    // 解析后原数据就已经不需要了
    free(buffer.data);
    // 不合法的查询包不作处理
    if (packet.header.qr != QRQUERY || packet.header.questionCount < 1) {
        DNSPacket_destroy(packet); //销毁packet，解除内存占用
        return;
    }
    /*
     * 由于多个question会产生歧义，且查阅到开源DNS系统Bind也不支持多个question
     * 故直接对多个question的DNS包做返回格式错误处理
     * ref: https://gitlab.isc.org/isc-projects/bind9/-/blob/main/lib/ns/query.c#L11977-11983
     */
    if (packet.header.questionCount > 1) {
        if (runtime->config.debug) {
            printf("Too many questions. \n");
        }
        packet.header.qr = QRRESPONSE;
        packet.header.rcode = FORMERR;
        buffer = DNSPacket_encode(packet);
        DNSPacket_destroy(packet);
        sendto(runtime->server, (char *)buffer.data, buffer.length, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        return;
    }
    // 若能查询先查询本地缓存
    if (checkCacheable(packet.questions->qtype)) {
        Key key; //
        key.qtype = packet.questions->qtype;
        strcpy_s(key.name, 256, packet.questions[0].name);
        MyData myData = lRUCacheGet(runtime->lruCache, key);
        uint32_t cacheTime = (uint32_t)(time(NULL) - myData.time);
        if (myData.answerCount > 0) {
            if (runtime->config.debug) {
                printf("HIT CACHE\n");
            }
            int TTLTimeout = 0;
            for (uint16_t i = 0; i < myData.answerCount; i++) {
                if (myData.time > 0 && cacheTime > myData.answers[i].ttl) {
                    // 任何一个过期都算过期
                    TTLTimeout = 1;
                    break;
                }
            }
            if (!TTLTimeout) { //未过期
                packet.header.rcode = OK;
                packet.header.qr = QRRESPONSE;
                packet.header.answerCount = myData.answerCount;
                packet.answers = (DNSRecord *)malloc(sizeof(DNSRecord) * myData.answerCount);
                for (uint16_t i = 0; i < myData.answerCount; i++) {
                    if (myData.answers[i].rdata == NULL) {
                        free(packet.answers);
                        packet.answers = NULL;
                        packet.header.answerCount = 0;
                        packet.header.rcode = NXDOMAIN;
                        break;
                    }
                    packet.answers[i] = myData.answers[i];
                    memcpy(packet.answers[i].name, myData.answers[i].name, sizeof(char) * (strlen(myData.answers[i].name) + 1));
                    packet.answers[i].rdata = (char *)malloc(packet.answers[i].rdataLength);
                    memcpy(packet.answers[i].rdata, myData.answers[i].rdata, packet.answers[i].rdataLength);
                    packet.answers[i].ttl = myData.time > 0 ? myData.answers[i].ttl - cacheTime : 0;
                    if (myData.answers[i].rdataName) {
                        memcpy(packet.answers[i].rdataName, myData.answers[i].rdataName, sizeof(char) * (strlen(myData.answers[i].rdataName) + 1));
                    } else {
                        strcpy_s(packet.answers[i].rdataName, 256, "");
                    }
                }
                if (runtime->config.debug) {
                    char clientIp[16];
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
                    printf("C<< Send packet back to client %s:%d\n", clientIp, ntohs(clientAddr.sin_port));
                    DNSPacket_print(&packet);
                    runtime->totalCount++;
                    printf("TOTAL COUNT %d\n", runtime->totalCount);
                    printf("CACHE SIZE %d\n", runtime->lruCache->size);
                }
                packet.header.ra = 1;
                buffer = DNSPacket_encode(packet);
                DNSPacket_destroy(packet);
                status = sendto(runtime->server, (char *)buffer.data, buffer.length, 0, (struct sockaddr *)&clientAddr, sizeof(runtime->upstreamAddr));
                free(buffer.data);
                if (status < 0) {
                    printf("Error sendto: %d\n", WSAGetLastError());
                }
                return;
            } else {
                printf("CACHE TIMEOUT\n");
            }
        }
    }
    // 缓存未命中或者不支持，转走
    // ID转换
    IdMap mapItem;
    mapItem.addr = clientAddr;
    mapItem.originalId = packet.header.id;
    mapItem.time = time(NULL) + IDMAP_TIMEOUT;
    runtime->maxId = setIdMap(runtime->idmap, mapItem, runtime->maxId);
    packet.header.id = runtime->maxId;
    // 发走
    if (runtime->config.debug) {
        printf("S<< Send packet to upstream\n");
        DNSPacket_print(&packet);
    }
    buffer = DNSPacket_encode(packet);
    DNSPacket_destroy(packet);
    status = sendto(runtime->client, (char *)buffer.data, buffer.length, 0, (struct sockaddr *)&runtime->upstreamAddr, sizeof(runtime->upstreamAddr));
    free(buffer.data);
    if (status < 0) {
        printf("Error sendto: %d\n", WSAGetLastError());
    }
}
/*
 * 接收上游应答
 */
void recvFromUpstream(DNSRD_RUNTIME *runtime) {
    Buffer buffer = makeBuffer(DNS_PACKET_SIZE);
    int status = 0;
    DNSPacket packet = recvDNSPacket(runtime, runtime->client, &buffer, &runtime->upstreamAddr, &status);
    // 解析后原数据就已经不需要了
    // 还原id
    IdMap client = getIdMap(runtime->idmap, packet.header.id);
    packet.header.id = client.originalId;
    // 发走
    if (runtime->config.debug) {
        char clientIp[16];
        inet_ntop(AF_INET, &client.addr.sin_addr, clientIp, sizeof(clientIp));
        printf("C<< Send packet back to client %s:%d\n", clientIp, ntohs(client.addr.sin_port));
        DNSPacket_print(&packet);
        runtime->totalCount++;
        printf("TOTAL COUNT %d\n", runtime->totalCount);
    }
    Buffer bufferTmp;
    bufferTmp = DNSPacket_encode(packet);
    for (int i = 0; i < 16; i++) {
        buffer.data[i] = bufferTmp.data[i];
    }
    free(bufferTmp.data);
    status = sendto(runtime->server, (char *)buffer.data, buffer.length, 0, (struct sockaddr *)&client.addr, sizeof(client.addr));
    if (status < 0) {
        printf("Error sendto: %d\n", WSAGetLastError());
    }
    int shouldCache = 1;
    if (packet.header.rcode != OK || !checkCacheable(packet.questions->qtype)) {
        shouldCache = 0;
    }
    if (shouldCache) {
        // 进缓存
        Key cacheKey;
        cacheKey.qtype = packet.questions->qtype;
        strcpy_s(cacheKey.name, 256, packet.questions->name);
        MyData cacheItem;
        cacheItem.time = time(NULL);
        cacheItem.answerCount = packet.header.answerCount;
        cacheItem.answers = (DNSRecord *)malloc(sizeof(DNSRecord) * packet.header.answerCount);
        for (uint16_t i = 0; i < packet.header.answerCount; i++) {
            DNSRecord *newRecord = &cacheItem.answers[i];
            DNSRecord *old = &packet.answers[i];
            newRecord->ttl = old->ttl;
            newRecord->type = old->type;
            newRecord->rclass = old->rclass;
            strcpy_s(newRecord->name, 256, old->name);
            if (strlen(old->rdataName)) {
                newRecord->rdata = (char *)malloc(sizeof(char) * 256);
                strcpy_s(newRecord->rdataName, 256, old->rdataName);
                toQname(old->rdataName, newRecord->rdata);
                newRecord->rdataLength = (uint16_t)strnlen_s(newRecord->rdata, 256) + 1;
            } else {
                newRecord->rdataLength = old->rdataLength;
                newRecord->rdata = (char *)malloc(sizeof(char) * newRecord->rdataLength);
                memcpy(newRecord->rdata, old->rdata, newRecord->rdataLength);
                newRecord->rdataName[0] = '\0';
            }
        }
        lRUCachePut(runtime->lruCache, cacheKey, cacheItem);
        writeCache(runtime->config.cachefile, runtime);
        if (runtime->config.debug) {
            printf("ADDED TO CACHE\n");
        }
    }
    if (runtime->config.debug) {
        printf("CACHE SIZE %d\n", runtime->lruCache->size);
    }
    // 用完销毁
    free(buffer.data);
    DNSPacket_destroy(packet);
}
/*
 * 主循环
 */
void loop(DNSRD_RUNTIME *runtime) {
    fd_set fd; //句柄的集合
    while (1) {
        FD_ZERO(&fd);                                   //清空句柄集合
        FD_SET(runtime->server, &fd);                   //加入句柄集合
        FD_SET(runtime->client, &fd);                   //
        TIMEVAL timeout = {5, 0};                       //5秒0
        int ret = select(0, &fd, NULL, NULL, &timeout); //返回状态
        if (runtime->quit == 1) {
            // 退出则不再处理
            return;
        }
        if (ret < 0) {
            // 小于0是出错了
            printf("ERROR: select %d\n", WSAGetLastError());
        } else if (ret > 0) {
            // 大于0说明有数据可以获取
            if (FD_ISSET(runtime->server, &fd)) { //收到了来自下级的请求
                recvFromClient(runtime);
            }
            if (FD_ISSET(runtime->client, &fd)) { //收到了来自上级的回复
                recvFromUpstream(runtime);
            }
        }
    }
}
