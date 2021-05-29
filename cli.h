#ifndef CLI_H
#define CLI_H
#include "RBTree.h"
#include "idTransfer.h"
#include <WinSock2.h>
typedef struct DNSRD_CONFIG {
    int debug;
    int port;
    char upstream[16];
    char hostfile[256];
} DNSRD_CONFIG;
typedef struct DNSRD_RUNTIME {
    DNSRD_CONFIG config;
    SOCKET server;
    SOCKET client;
    IdMap *idmap;
    uint16_t maxId;
    struct Node *rbtree;
    struct sockaddr_in listenAddr;
    struct sockaddr_in upstreamAddr;
    int quit;
} DNSRD_RUNTIME;
DNSRD_CONFIG initConfig(int argc, char *argv[]);
DNSRD_RUNTIME initRuntime(DNSRD_CONFIG *config);
void destroyRuntime(DNSRD_RUNTIME *runtime);
void cliHelp();
void cliHead();
void cliStarted(DNSRD_CONFIG *config);
#endif