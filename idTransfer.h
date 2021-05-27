#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#define MAXID 65535

typedef struct idMap
{
    uint32_t time;
    uint16_t originalId;
    sockaddr_in addr;
} IdMap;

IdMap *initIdMap();
IdMap getIdMap(IdMap *idMap, uint16_t i);
int setIdMap(IdMap *idMap, IdMap item, uint16_t curMaxId);