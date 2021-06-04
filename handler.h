#ifndef HANDLER_H
#define HANDLER_H
#include "DNSPacket.h"
#include "cli.h"
void initSocket(DNSRD_RUNTIME *runtime);
void loop(DNSRD_RUNTIME *runtime);
#endif