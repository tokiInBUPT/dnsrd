#include "idTransfer.h"

IdMap *initIdMap() {
    IdMap *tmp = (IdMap *)malloc(sizeof(IdMap) * MAXID + 1);
    for (int i = 0; i < MAXID; i++) {
        tmp[i].time = 0;
    }
    return tmp;
}

IdMap getIdMap(IdMap *idMap, uint16_t i) {
    idMap[i].time = 0;
    return idMap[i];
}

int setIdMap(IdMap *idMap, IdMap item, uint16_t curMaxId) {
    uint16_t originId = curMaxId;
    time_t t = time(NULL);
    while (idMap[curMaxId].time >= t) {
        curMaxId++;
        if (curMaxId == originId) {
            return -1;
        }
    }
    idMap[curMaxId] = item;
    return curMaxId;
}