#include "cli.h"
#include "handler.h"
#include <signal.h>
#include <stdio.h>
#include <windows.h>

DNSRD_RUNTIME runtime;

/* 按Ctrl+C退出时需要释放内存 */
BOOL WINAPI consoleHandler(DWORD signal) {
    printf("Quitting...");
    destroyRuntime(&runtime);
    exit(0);
}

int main(int argc, char *argv[]) {
    cliHead();//打印标题
    /* 从命令行参数获取配置信息  */
    DNSRD_CONFIG config = initConfig(argc, argv); 
    /* 初始化ID转换表和红黑树  */
    runtime = initRuntime(&config);
    /* 初始化Socket */
    initSocket(&runtime);
    /* 输出初始化信息 */
    cliStarted(&config);
    /* 设置Ctrl+C(SIGINT)时的友好退出  */
    SetConsoleCtrlHandler(consoleHandler, TRUE);
    /* 开始循环 */
    loop(&runtime);
    /* 错误退出也释放内存  */
    destroyRuntime(&runtime);
    return 0;
}
