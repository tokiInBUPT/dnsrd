#ifndef CLI_H
#define CLI_H
#include "idTransfer.h"
#include "lhm.h"
#include <WinSock2.h>
#define MAXCACHE 128
typedef struct DNSRD_CONFIG {
    int debug;
    int port;
    char upstream[16];
    char hostfile[256];
    char cachefile[256];
} DNSRD_CONFIG;
typedef struct DNSRD_RUNTIME {
    DNSRD_CONFIG config;
    SOCKET server;
    SOCKET client;
    IdMap *idmap;
    uint16_t maxId;
    LRUCache *lruCache;
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