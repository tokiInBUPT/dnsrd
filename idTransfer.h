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
    uint32_t time;
    uint16_t originalId;
    struct sockaddr_in addr;
} IdMap;

IdMap *initIdMap();
IdMap getIdMap(IdMap *idMap, uint16_t i);
int setIdMap(IdMap *idMap, IdMap item, uint16_t curMaxId);
#endif