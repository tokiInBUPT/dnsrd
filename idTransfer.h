#ifndef IDTRANSFER_H
#define IDTRANSFER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>

#define MAXID 65535

typedef struct idMap {
    time_t time;             //请求时间
    uint16_t originalId;     //请求方ID
    struct sockaddr_in addr; //请求方IP+端口
} IdMap;

IdMap *initIdMap();
IdMap getIdMap(IdMap *idMap, uint16_t i);
int setIdMap(IdMap *idMap, IdMap item, uint16_t curMaxId);
#endif