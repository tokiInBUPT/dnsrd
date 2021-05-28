#include "cli.h"
#include "lib/getopt.h"
#include "loadConfig.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
DNSRD_CONFIG initConfig(int argc, char *argv[]) {
    DNSRD_CONFIG config;
    config.upstream[0] = '\0';
    config.hostfile[0] = '\0';
    config.port = 53;
    config.debug = 0;
    char current = '\0';
    while ((current = getopt(argc, argv, "u:f:p:dh")) != -1) {
        switch (current) {
        case 'h':
            cliHelp();
            exit(-1);
            break;
        case 'd':
            config.debug = 1;
            break;
        case 'f':
            strcpy_s(config.hostfile, 255, optarg);
            break;
        case 'u':
            strcpy_s(config.upstream, 15, optarg);
            break;
        case 'p':
            config.port = atoi(optarg);
            break;
        }
    }
    if (strnlen_s(config.upstream, 15) == 0 && strnlen_s(config.hostfile, 15) == 0) {
        cliHelp();
        exit(-1);
    }
    if (strnlen_s(config.upstream, 15) <= 7) {
        printf("Invaild upstream ip");
        exit(-1);
    }
    if (_access(config.hostfile, 0) == -1) {
        printf("Invaild hostfile");
        exit(-1);
    }
    if (config.port <= 0) {
        printf("Invaild port");
        exit(-1);
    }
    return config;
}
DNSRD_RUNTIME initRuntime(DNSRD_CONFIG *config) {
    DNSRD_RUNTIME runtime;
    runtime.idmap = initIdMap();
    runtime.rbtree = NULL;
    loadConfig(config->hostfile, &runtime.rbtree);
    return runtime;
}
void destroyRuntime(DNSRD_RUNTIME *runtime) {
    free(runtime->idmap);
    deleteAll(runtime->rbtree);
}
void cliHelp() {
    printf("DNSRD Project v1.0 by xyToki&Jray&cn_zyk 2021.5\n");
    printf("Usage: dnsrd -u <upstream> [-f <hostfile>] [-p <port>] [-d] [-h]\n");
    printf("       -u upstream    IP of upstream dns server. e.g. 1.1.1.1\n");
    printf("       -f hostfile    Path to host file. e.g. hosts.txt \n");
    printf("       -p port        Port of this dns server. 53 by default. \n");
    printf("       -d             Enable debug logging. \n");
    printf("       -h             Print this help text. \n");
}