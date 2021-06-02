#include "handler.h"
#include "cli.h"
#include "config.h"
#include "idTransfer.h"
#include <WS2tcpip.h>
#include <WinSock2.h>
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
}
/*
 * 通用接收函数
 */
DNSPacket recvDNSPacket(DNSRD_RUNTIME *runtime, SOCKET socket, Buffer *buffer, struct sockaddr_in *clientAddr, int *error) {
    int clientAddrLength = sizeof(*clientAddr);
    int ret = recvfrom(socket, (char *)buffer->data, buffer->length, 0, (struct sockaddr *)clientAddr, &clientAddrLength);
    if (ret < 0) {
        printf("Error: recvfrom %d\n", WSAGetLastError());
        *error = -1;
        DNSPacket packet;
        packet.answers = NULL;
        packet.questions = NULL;
        packet.additional = NULL;
        packet.authorities = NULL;
        packet.header.questionCount = 0;
        packet.header.answerCount = 0;
        packet.header.additionalCount = 0;
        packet.header.authorityCount = 0;
        return packet;
    } else if (ret == 0) {
        printf("Error: recvfrom 0\n");
        *error = 0;
        DNSPacket packet;
        packet.answers = NULL;
        packet.questions = NULL;
        packet.additional = NULL;
        packet.authorities = NULL;
        packet.header.questionCount = 0;
        packet.header.answerCount = 0;
        packet.header.additionalCount = 0;
        packet.header.authorityCount = 0;
        return packet;
    }
    buffer->length = ret;
    *error = ret;
    DNSPacket packet = DNSPacket_decode(*buffer);
    if (runtime->config.debug) {
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
    Buffer buffer = makeBuffer(4096);
    struct sockaddr_in clientAddr;
    int status = 0;
    DNSPacket packet = recvDNSPacket(runtime, runtime->server, &buffer, &clientAddr, &status);
    if (status <= 0) {
        // 接收失败 - 空包，甚至不需要destroy。
        return;
    }
    // 解析后原数据就已经不需要了
    free(buffer.data);
    // 不合法的查询包不作处理
    if (packet.header.qr != QRQUERY || packet.header.questionCount < 1) {
        DNSPacket_destroy(packet);
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
        Key key;
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
            if (!TTLTimeout) {
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
                    packet.answers[i].name = (char *)malloc(sizeof(char) * (strlen(myData.answers[i].name) + 1));
                    memcpy(packet.answers[i].name, myData.answers[i].name, sizeof(char) * (strlen(myData.answers[i].name) + 1));
                    packet.answers[i].rdata = (char *)malloc(packet.answers[i].rdataLength);
                    memcpy(packet.answers[i].rdata, myData.answers[i].rdata, packet.answers[i].rdataLength);
                    packet.answers[i].ttl = myData.time > 0 ? myData.answers[i].ttl - cacheTime : 0;
                    if (myData.answers[i].rdataName) {
                        packet.answers[i].rdataName = (char *)malloc(sizeof(char) * (strlen(myData.answers[i].rdataName) + 1));
                        memcpy(packet.answers[i].rdataName, myData.answers[i].rdataName, sizeof(char) * (strlen(myData.answers[i].rdataName) + 1));
                    } else {
                        packet.answers[i].rdataName = NULL;
                    }
                }
                if (runtime->config.debug) {
                    char clientIp[16];
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
                    printf("C<< Send packet back to client %s:%d\n", clientIp, ntohs(clientAddr.sin_port));
                    DNSPacket_print(&packet);
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
    Buffer buffer = makeBuffer(512);
    int status = 0;
    DNSPacket packet = recvDNSPacket(runtime, runtime->client, &buffer, &runtime->upstreamAddr, &status);
    // 解析后原数据就已经不需要了
    free(buffer.data);
    // 还原id
    IdMap client = getIdMap(runtime->idmap, packet.header.id);
    packet.header.id = client.originalId;
    // 发走
    if (runtime->config.debug) {
        char clientIp[16];
        inet_ntop(AF_INET, &client.addr.sin_addr, clientIp, sizeof(clientIp));
        printf("C<< Send packet back to client %s:%d\n", clientIp, ntohs(client.addr.sin_port));
        DNSPacket_print(&packet);
    }
    buffer = DNSPacket_encode(packet);
    status = sendto(runtime->server, (char *)buffer.data, buffer.length, 0, (struct sockaddr *)&client.addr, sizeof(client.addr));
    if (status < 0) {
        printf("Error sendto: %d\n", WSAGetLastError());
    }
    int shouldCache = 0;
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
            size_t nameLen = strnlen_s(old->name, 256);
            newRecord->name = (char *)malloc(sizeof(char) * (nameLen + 1));
            strcpy_s(newRecord->name, nameLen + 1, old->name);
            if (old->rdataName != NULL) {
                nameLen = strnlen_s(old->rdataName, 256);
                newRecord->rdata = (char *)malloc(sizeof(char) * (nameLen + 2));
                newRecord->rdataName = (char *)malloc(sizeof(char) * (nameLen + 1));
                strcpy_s(newRecord->rdataName, nameLen + 1, old->rdataName);
                toQname(old->rdataName, newRecord->rdata);
                newRecord->rdataLength = (uint16_t)strnlen_s(newRecord->rdata, 256) + 1;
            } else {
                newRecord->rdataLength = old->rdataLength;
                newRecord->rdata = (char *)malloc(sizeof(char) * newRecord->rdataLength);
                memcpy(newRecord->rdata, old->rdata, newRecord->rdataLength);
                newRecord->rdataName = NULL;
            }
        }
        lRUCachePut(runtime->lruCache, cacheKey, cacheItem);
    }
    // 用完销毁
    free(buffer.data);
    DNSPacket_destroy(packet);
}
/*
 * 主循环
 */
void loop(DNSRD_RUNTIME *runtime) {
    fd_set fd;
    while (1) {
        FD_ZERO(&fd);
        FD_SET(runtime->server, &fd);
        FD_SET(runtime->client, &fd);
        TIMEVAL timeout = {5, 0};
        int ret = select(0, &fd, NULL, NULL, &timeout);
        if (runtime->quit == 1) {
            // 退出则不再处理
            return;
        }
        if (ret < 0) {
            // 小于0是出错了
            printf("ERROR: select %d\n", WSAGetLastError());
        } else if (ret > 0) {
            // 大于0说明有数据可以获取
            if (FD_ISSET(runtime->server, &fd)) {
                recvFromClient(runtime);
            }
            if (FD_ISSET(runtime->client, &fd)) {
                recvFromUpstream(runtime);
            }
        }
    }
}