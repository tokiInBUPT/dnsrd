#ifndef CLI_H
#define CLI_H
#include "idTransfer.h"
#include "lhm.h"
#include <WinSock2.h>
#define MAXCACHE 13
typedef struct DNSRD_CONFIG {
    int debug;          //是否输出debug信息
    int port;           //监听地址，默认53
    char upstream[16];  //上级DNS服务器的IP
    char hostfile[256]; //HOST文件的文件名
    char cachefile[256];
} DNSRD_CONFIG;
typedef struct DNSRD_RUNTIME {
    DNSRD_CONFIG config;             //服务器配置信息
    SOCKET server;                   //接受请求的socket
    SOCKET client;                   //与上级连接的socket
    IdMap *idmap;                    //ID转换表 （ID：对一次DNS请求的标识，用于确定请求方）
    uint16_t maxId;                  //上一次请求上级时所使用的ID号
    LRUCache *lruCache;              //缓存来自上级的查询结果，如果请求在缓存内，则直接回复
    struct sockaddr_in listenAddr;   //监听地址（请求方地址IP+端口
    struct sockaddr_in upstreamAddr; //上级DNS服务器地址（IP+端口）
    int quit;                        //退出是否已经被触发
} DNSRD_RUNTIME;
DNSRD_CONFIG initConfig(int argc, char *argv[]);
DNSRD_RUNTIME initRuntime(DNSRD_CONFIG *config);
void destroyRuntime(DNSRD_RUNTIME *runtime);
void cliHelp();
void cliHead();
void cliStarted(DNSRD_CONFIG *config);
#endif