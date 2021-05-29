#include "handler.h"
#include "DNSPacket.h"
#include "RBTree.h"
#include "cli.h"
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
    if (bind(runtime->server, (SOCKADDR *)&runtime->listenAddr, sizeof(runtime->listenAddr)) < 0) {
        printf("ERROR: bind faild: %d\n", errno);
        exit(-1);
    }
    /* 上游socket  */
    runtime->client = socket(AF_INET, SOCK_DGRAM, 0);
    runtime->upstreamAddr.sin_family = AF_INET;
    inet_pton(AF_INET, runtime->config.upstream, &runtime->upstreamAddr.sin_addr);
    runtime->listenAddr.sin_port = htons(53);
}
/*
 * 接收客户端的查询请求
 */
void recvFromClient(DNSRD_RUNTIME *runtime) {
    Buffer buffer = makeBuffer(512);
    struct sockaddr_in clientAddr;
    int clientAddrLength = 0;
    buffer.length = recvfrom(runtime->server, (char *)buffer.data, buffer.length, 0, (struct sockaddr *)&clientAddr, &clientAddrLength);
    DNSPacket packet = DNSPacket_decode(buffer);
}
/*
 * 接收上游应答
 */
void recvFromUpstream(DNSRD_RUNTIME *runtime) {}
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