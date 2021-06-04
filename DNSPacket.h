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
    QRQUERY,   //DNSpacket类型是query
    QRRESPONSE //DNSpacket类型是response
} DNSPacketQR;

typedef enum {
    QUERY,  //标准查询
    IQUERY, //反向查询
    STATUS  //服务器状态请求
} DNSPacketOP;

typedef enum {
    OK,        //无错误
    FORMERR,   //报文格式错误
    SERVFAIL,  //域名服务器失败
    NXDOMAIN,  //名字错误
    NOTIMP,    //查询类型不支持
    REFUSED    //查询请求被拒绝
} DNSPacketRC; //表示响应的差错状态

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
    DNSPacketRC rcode;        //返回码字段（表示响应的状态）
    uint16_t questionCount;   //问题计数（不支持）
    uint16_t answerCount;     //回答资源记录数
    uint16_t authorityCount;  //权威名称服务器计数
    uint16_t additionalCount; //附加资源记录数
} DNSPacketHeader;

typedef enum {
    A = 0x01,     //主机地址
    NS = 0x02,    //权威服务器
    CNAME = 0x05, //域名别名
    SOA = 0x06,   //作为权威区数据的起始资源类型
    NUL = 0x0A,   //空
    PTR = 0x0C,   //域名指针
    MX = 0x0F,    //邮件交换地址
    TXT = 0x10,   //文本符号
    AAAA = 0x1c,  //IPv6地址
    ANY = 0xFF,   //所有
    OPT = 41      //EDNS
} DNSQType;       //DNS请求类型

/* 目前其它选项基本已经排除 */
typedef enum {
    DNS_IN = 0x01, //互联网查询
} DNSQClass;

typedef struct DNSQuestion {
    DNSQType qtype;   //DNS请求的资源类型
    DNSQClass qclass; //地址类型，通常为互联网查询，值为1
    char name[256];   //所查询的域名字符串
} DNSQuestion;

typedef struct DNSRecord {
    /* 正常名称 - 转换后再存入 */
    char name[256];       //DNS请求的域名
    DNSQType type;        //资源记录的类型
    DNSQClass rclass;     //地址类型（与问题中一致）
    uint32_t ttl;         //有效时间，即请求方
    uint16_t rdataLength; //rdata的长度
    char *rdata;          //资源数据（可能是answers、authorities或者additional的信息）
    char rdataName[256];  //某些资源需要decode成字符串
} DNSRecord;              //DNS资源记录

typedef struct DNSPacket {
    DNSPacketHeader header; //packet信息（整体）
    DNSQuestion *questions; //query信息
    DNSRecord *answers;     //请求响应
    DNSRecord *authorities; //权威服务器名称
    DNSRecord *additional;  //附加信息
} DNSPacket;                //DNS packet

typedef struct Buffer {
    uint8_t *data;   //buffer首地址
    uint32_t length; //buffer长度
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
DNSPacket DNSPacket_decode(Buffer *buffer);
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