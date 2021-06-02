#ifndef CONFIG_H
#define CONFIG_H

#define IDMAP_TIMEOUT 5
/*
 * 考虑到客户端可能会携带edns包等，我们最大支持接收4096
 */
#define DNS_PACKET_SIZE 4096
/*
 * 发送超过512时，理论应该标注TC，若考虑EDNS推荐值则应该是1232
 * ref: https://dnsflagday.net/2020/
 */
#define DNS_PACKET_TC 512
#define DNS_PACKET_TC_EDNS 1232

#endif