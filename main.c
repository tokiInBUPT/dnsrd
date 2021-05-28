#include "DNSQuery.h"
#include "cli.h"
#include <WinSock2.h>
#include <signal.h>
#include <stdio.h>
#include <windows.h>

DNSRD_CONFIG config;
DNSRD_RUNTIME runtime;

/* 按Ctrl+C退出时需要释放内存 */
static void DNSRD_exit(int sig) {
    destroyRuntime(&runtime);
    exit(0);
}

int main(int argc, char *argv[]) {
    /* 初始化WinSock 2.2  */
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    /* 从命令行参数获取配置信息  */
    config = initConfig(argc, argv);
    /* 初始化ID转换表和红黑树  */
    runtime = initRuntime(&config);
    /* 初始化Socket */
    /* 开始循环 */
    /* 设置Ctrl+C(SIGINT)时的友好退出  */
    signal(SIGINT, DNSRD_exit);
    /* 错误退出也释放内存  */
    destroyRuntime(&runtime);
    return 0;
}
