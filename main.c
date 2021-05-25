#include "DNSQuery.h"
#include <WinSock2.h>>
#include <stdio.h>

int main(int argc, char *argv[]) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    char hostname[100];
    char dns_servername[100];
    printf("DNSIP>> ");
    scanf("%s", dns_servername);
    printf("HOST>> ");
    scanf("%s", hostname);
    DNSPacket res = DNSQuery(dns_servername, hostname, A);
    return 0;
}
