#ifndef LOADCACHE_H
#define LOADCACHE_H
#include "cli.h"
#include "lib/yyjson.h"

void writeCache(char *fname, DNSRD_RUNTIME *runtime);
void loadCache(char *fname, DNSRD_RUNTIME *runtime);
#endif