/*
 * DNS数据包封装 / 解封 / 复制
 * @xyToki
 */
#ifndef DNSPACKET_H
#define DNSPACKET_H
#include <stdint.h>
// c最小单位是b8it
typedef uint8_t bit;
typedef enum {
    QRQUERY,
    QRRESPONSE
} DNSPacketQR;

typedef enum {
    QUERY,
    IQUERY,
    STATUS
} DNSPacketOP;

typedef enum {
    OK,
    FORMERR,
    SERVFAIL,
    NXDOMAIN,
    NOTIMP,
    REFUSED
} DNSPacketRC;

typedef struct DNSPacketHeader {
    /*
     * [ALL]
     * Session ID
     * 16bit
     */
    uint16_t id;
    /* 
     * [ALL]
     * QUERY or RESPONSE
     * 1bit
    */
    DNSPacketQR qr;
    /* 
     * [ALL]
     * Operation
     * QUERY/IQUERY/STATUS
     * 4bit
    */
    DNSPacketOP opcode;
    /* 
     * [RESPONSE]
     * Authoritative
     * 1bit
    */
    bit aa;
    /* 
     * [RESPONSE]
     * Message truncated
     * 1bit
    */
    bit tc;
    /* 
     * [REQUEST]
     * Recursion Desired
     * 1 - recursively
     * 0 - direct
     * 1bit
    */
    bit rd;
    /* 
     * [RESPONSE]
     * Recursion Answer
     * 1 - recursively
     * 0 - direct
     * 1bit
    */
    bit ra;
    /* 
     * [RESPONSE]
     * Response code
     * OK/EFORMAT/ESERVER/ENAME/EIMPL/EREFUSE
     * 4bit
    */
    DNSPacketRC rcode;
    uint16_t questionCount;
    uint16_t answerCount;
    uint16_t authorityCount;
    uint16_t additionalCount;
} DNSPacketHeader;

typedef enum {
    A = 0x01,
    NS = 0x02,
    CNAME = 0x05,
    SOA = 0x06,
    NUL = 0x0A,
    PTR = 0x0C,
    MX = 0x0F,
    TXT = 0x10,
    AAAA = 0x1c,
    ANY = 0xFF,
    OPT = 41
} DNSQType;

/* 目前其它选项基本已经排除 */
typedef enum {
    DNS_IN = 0x01,
} DNSQClass;

typedef struct DNSQuestion {
    DNSQType qtype;
    DNSQClass qclass;
    /* 正常名称 - 发送后再转换 */
    char name[256];
} DNSQuestion;

typedef struct DNSRecord {
    /* 正常名称 - 转换后再存入 */
    char name[256];
    DNSQType type;
    DNSQClass rclass;
    uint32_t ttl;
    uint16_t rdataLength;
    char *rdata;
    char rdataName[256];
} DNSRecord;

typedef struct DNSPacket {
    DNSPacketHeader header;
    DNSQuestion *questions;
    DNSRecord *answers;
    DNSRecord *authorities;
    DNSRecord *additional;
} DNSPacket;

typedef struct Buffer {
    uint8_t *data;
    uint32_t length;
} Buffer;
Buffer makeBuffer(int len);
/*
 * 结构体转buffer
 */
Buffer DNSPacket_encode(DNSPacket packet);
void DNSPacket_destroy(DNSPacket packet);
/*
 * Buffer转结构体
 */
DNSPacket DNSPacket_decode(Buffer buffer);
/*
 * 填充发送包的基本属性
 */
void DNSPacket_fillQuery(DNSPacket *packet);
/*
 * 调试输出一个DNS包的内容
 */
void DNSPacket_print(DNSPacket *packet);
int toQname(char *str, char *newStr);
#endif