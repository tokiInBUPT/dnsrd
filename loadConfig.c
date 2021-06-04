#include "loadConfig.h"
#include "DNSPacket.h"
#include "string.h"
#include <ws2tcpip.h>

void loadConfig(char *fname, LRUCache *lruCache) {
    FILE *file = fopen(fname, "r");
    char buf[MAX_BUF_LEN];
    while (fgets(buf, MAX_BUF_LEN, file) != NULL) {
        if (buf[0] == '#' || buf[0] == '\n') {
            continue;
        } else {
            char buf1[MAX_BUF_LEN];
            char buf2[MAX_BUF_LEN];
            char *bufs[] = {buf1, buf2};
            int flag = 0;
            int i = 0, j = 0;
            int flag_v6 = 0;
            int flag_cname = 0;
            for (i = 0; i < strlen(buf); i++) {
                if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '#' && buf[i] != '\n') {
                    bufs[flag][i - j] = buf[i];
                } else if (flag == 0) {
                    bufs[flag][i - j] = 0;
                    flag++;
                    j = i + 1;
                } else {
                    bufs[flag][i - j] = 0;
                    break;
                }
                if (buf[i] == ':') {
                    flag_v6 = 1;
                }
                if (!flag && ((buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= 'A' && buf[i] <= 'Z'))) {
                    flag_cname = 1;
                }
            }
            bufs[flag][i - j] = 0;
            Key key;
            if (flag_v6) {
                key.qtype = AAAA;
            } else if (!flag_v6 && flag_cname) {
                key.qtype = CNAME;
            } else {
                key.qtype = A;
            }
            strcpy_s(key.name, 256, buf2);
            MyData myData;
            DNSRecord record;
            record.type = key.qtype;
            record.ttl = 0;
            if (flag_v6) {
                record.rdata = (char *)malloc(sizeof(uint8_t) * 16);
                struct in6_addr ipv6data;
                inet_pton(AF_INET6, buf1, &ipv6data);
                int flag_nxdomain = 1;
                for (int i = 0; i < 16; i++) {
                    if (*((char *)(ipv6data.u.Byte + i)) != 0) {
                        flag_nxdomain = 0;
                        break;
                    }
                }
                if (flag_nxdomain) {
                    record.rdata = NULL;
                    record.rdataLength = 0;
                } else {
                    memcpy(record.rdata, ipv6data.u.Byte, sizeof(uint8_t) * 16);
                    record.rdataLength = sizeof(uint8_t) * 16;
                }
            } else if (!flag_v6 && flag_cname) {
                record.rdata = (char *)malloc(257);
                char tmp[257];
                strcpy_s(tmp, 257, buf1);
                strcpy_s(record.rdataName, 257, buf1);
                toQname(tmp, record.rdata);
                record.rdataLength = strnlen_s(buf1, 256) + 2;
                if (strstr(buf2, ".in-addr.arpa") != NULL) {
                    key.qtype = PTR;
                    record.type = PTR;
                }
            } else {
                record.rdata = (char *)malloc(sizeof(uint8_t) * 4);
                struct in_addr ipv4data;
                inet_pton(AF_INET, buf1, &ipv4data);
                if (ipv4data.S_un.S_addr == 0) {
                    record.rdata = NULL;
                    record.rdataLength = 0;
                } else {
                    memcpy(record.rdata, &ipv4data.S_un.S_addr, sizeof(int));
                    record.rdataLength = sizeof(uint8_t) * 4;
                }
            }
            strcpy_s(record.name, strnlen_s(buf2, 256) + 1, buf2);
            record.rclass = DNS_IN;
            strcpy_s(record.rdataName, 256, "");
            myData.answerCount = 1;
            myData.answers = (DNSRecord *)malloc(sizeof(DNSRecord));
            *myData.answers = record;
            myData.time = 0;
            lRUCachePut(lruCache, key, myData);
        }
    }
    printf("Config file loaded, cache size is %d\n", lruCache->size);
}