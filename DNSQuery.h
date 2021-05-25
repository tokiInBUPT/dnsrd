#ifndef DNSQUERY_H
#define DNSQUERY_H
#include "DNSPacket.h"
#include <winsock2.h>

DNSPacket DNSQuery(char *server, char *name, DNSQType type);
#endif