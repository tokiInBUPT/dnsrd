#include "DNSQuery.h"
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
DNSPacket DNSQuery(char *server, char *name, DNSQType type) {
    /* 封包 */
    DNSPacket query;
    DNSPacket_fillQuery(&query);
    query.questions = (DNSQuestion *)malloc(sizeof(DNSQuestion));
    query.questions->qtype = type;
    query.questions->qclass = DNS_IN;
    query.questions->name = (char *)malloc(sizeof(char) * 256);
    strcpy(query.questions->name, name);
    Buffer buffer = DNSPacket_encode(query);
    /* 发送 */
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr(server);
    int ret = sendto(s, (char *)buffer.data, buffer.length, 0, (struct sockaddr *)&dest, sizeof(dest));
    if (ret < 0) {
        int error = WSAGetLastError();
        printf("FAIL %d\n", error);
    }
    printf("SENT\n");
    int fromlen = sizeof(dest);
    ret = recvfrom(s, (char *)buffer.data, 512, 0, (struct sockaddr *)&dest, &fromlen);
    if (ret < 0) {
        int error = WSAGetLastError();
        printf("FAIL %d\n", error);
    }
    buffer.length = ret;
    printf("RSUCCESS\n");
    DNSPacket response = DNSPacket_decode(buffer);
    printf("PARSED\n");
    DNSPacket_destroy(query);
    free(buffer.data);
    return response;
}