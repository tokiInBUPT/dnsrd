#include "loadConfig.h"
#include <ws2tcpip.h>

void loadConfig(struct Node *rbTree)
{
    FILE *file = fopen("config.txt", "r");
    char buf[MAX_BUF_LEN];
    while (fgets(buf, MAX_BUF_LEN, file) != NULL)
    {
        if (buf[0] == '#' || buf[0] == '\n')
        {
            continue;
        }
        else
        {
            char buf1[MAX_BUF_LEN];
            char buf2[MAX_BUF_LEN];
            char *bufs[] = {buf1, buf2};
            int flag = 0;
            int j = 0;
            int flag_v6 = 0;
            for (int i = 0; i < strlen(buf); i++)
            {
                if (buf[i] != ' ' && buf[i] != '#' && buf[i] != '\n')
                {
                    bufs[flag][i - j] = buf[i];
                }
                else if (flag == 0)
                {
                    bufs[flag][i - j] = 0;
                    flag++;
                    j = i + 1;
                }
                else
                {
                    bufs[flag][i - j] = 0;
                    break;
                }
                if (buf[i] == ':')
                {
                    flag_v6 = 1;
                }
            }
            Key key;
            if (flag_v6)
            {
                key.qtype = AAAA;
            }
            else
            {
                key.qtype = A;
            }
            strcpy(key.name, buf2);
            MyData myData;
            DNSRecord record;
            record.type = key.qtype;
            record.ttl = 0;
            if (flag_v6)
            {
                record.rdata = (char *)malloc(sizeof(uint16_t) * 8);
                struct in6_addr ipv6data;
                inet_pton(AF_INET6, buf1, &ipv6data);
                strcpy(record.rdata, (char *)ipv6data.u.Byte);
                record.rdataLength = sizeof(uint16_t) * 8;
            }
            else
            {
                record.rdata = (char *)malloc(sizeof(uint16_t) * 4);
                struct in_addr ipv4data;
                inet_pton(AF_INET, buf1, &ipv4data);
                strcpy(record.rdata, (char *)ipv4data.S_un.S_addr);
                record.rdataLength = sizeof(uint16_t) * 8;
            }
            strcpy(record.name, buf2);
            myData.record = record;
            myData.time = 0;
            RB_insert(rbTree, key, myData);
        }
    }
}