#include "DNSPacket.h"
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
uint8_t *_read32(uint8_t *ptr, uint32_t *value) {
    *value = ntohl(*(uint32_t *)ptr);
    return ptr + 4;
}
uint8_t *_write32(uint8_t *ptr, uint32_t value) {
    *(uint32_t *)ptr = htonl(value);
    return ptr + 4;
}
uint8_t *_read16(uint8_t *ptr, uint16_t *value) {
    *value = ntohs(*(uint16_t *)ptr);
    return ptr + 2;
}
uint8_t *_write16(uint8_t *ptr, uint16_t value) {
    *(uint16_t *)ptr = htons(value);
    return ptr + 2;
}
uint8_t *_read8(uint8_t *ptr, uint8_t *value) {
    *value = *(uint8_t *)ptr;
    return ptr + 1;
}
uint8_t *_write8(uint8_t *ptr, uint8_t value) {
    *(uint8_t *)ptr = value;
    return ptr + 1;
}

int toQname(char *str, char *newStr) {
    int len = (int)strlen(str);
    newStr[0] = '.';
    strcpy(newStr + 1, str); // now newStr is like ".a.example.com"
    int dot = 0;
    for (int i = dot + 1; i <= len + 1; i++) {
        if (newStr[i] == '.' || i == len + 1) {
            uint8_t len = i - dot - 1;
            newStr[dot] = len;
            dot = i;
        }
    }
    return len + 2;
}
int fromQname(char *str, char *newStr) {
    int len = (int)strlen(str);
    if (len <= 0) {
        return 0;
    }
    int dot = 0;
    for (int i = 0; i <= len; i++) {
        if (i == dot) {
            dot = i + str[i] + 1;
            if (str[i] > 0) {
                str[i] = '.';
            }
        }
    }
    strcpy(newStr, str + 1);
    return len + 1;
}
int decodeQname(char *ptr, char *start, char *newStr) {
    uint16_t f = ptr[0] << 8 | ptr[1];
    if ((f & 0xc000) == 0xc000) {
        // 数据包压缩
        int offset = f & 0xfff;
        ptr = start + offset;
        fromQname(ptr, newStr);
        return 2;
    } else {
        return fromQname(ptr, newStr);
    }
}
Buffer DNSPacket_encode(DNSPacket packet) {
    Buffer buffer;
    uint8_t *data = (uint8_t *)malloc(512 * sizeof(uint8_t));
    buffer.data = data;
    /* Header */
    data = _write16(data, packet.header.id);
    /* QR+OP+AA+TC+RD */
    data = _write8(data,
                   packet.header.qr << 7 |
                       packet.header.opcode << 3 |
                       packet.header.aa << 2 |
                       packet.header.tc << 1 |
                       packet.header.rd << 0);
    /* RA+padding(3)+RCODE */
    data = _write8(data, packet.header.ra << 7 | packet.header.rcode);
    /* Counts */
    data = _write16(data, packet.header.questionCount);
    data = _write16(data, packet.header.answerCount);
    data = _write16(data, packet.header.authorityCount);
    data = _write16(data, packet.header.additionalCount);
    /* Questions */
    for (int i = 0; i < packet.header.questionCount; i++) {
        DNSQuestion *r = &packet.questions[i];
        data += toQname(r->name, (char *)data);
        data = _write16(data, r->qtype);
        data = _write16(data, r->qclass);
    }
    /* Answers */
    for (int i = 0; i < packet.header.answerCount; i++) {
        DNSRecord *r = &packet.answers[i];
        data += toQname(r->name, (char *)data);
        data = _write16(data, r->type);
        data = _write16(data, r->rclass);
        data = _write32(data, r->ttl);
        data = _write16(data, r->rdataLength);
    }
    /* Authorities */
    for (int i = 0; i < packet.header.authorityCount; i++) {
        DNSRecord *r = &packet.authorities[i];
        data += toQname(r->name, (char *)data);
        data = _write16(data, r->type);
        data = _write16(data, r->rclass);
        data = _write32(data, r->ttl);
        data = _write16(data, r->rdataLength);
        memcpy(data, r->rdata, r->rdataLength);
        data += r->rdataLength;
    }
    /* Additional */
    for (int i = 0; i < packet.header.additionalCount; i++) {
        DNSRecord *r = &packet.additional[i];
        data += toQname(r->name, (char *)data);
        data = _write16(data, r->type);
        data = _write32(data, r->ttl);
        data = _write16(data, r->rdataLength);
        memcpy(data, r->rdata, r->rdataLength);
        data += r->rdataLength;
    }
    buffer.length = (uint32_t)(data - buffer.data);
    return buffer;
}
DNSPacket DNSPacket_decode(Buffer buffer) {
    DNSPacket packet;
    uint8_t *data = buffer.data;
    uint8_t tmp8;
    data = _read16(data, &packet.header.id);
    /* QR+OP+AA+TC+RD */
    data = _read8(data, &tmp8);
    packet.header.qr = (DNSPacketQR)(tmp8 >> 7 & 0x01);
    packet.header.opcode = (DNSPacketOP)(tmp8 >> 3 & 0x0F);
    packet.header.aa = tmp8 >> 2 & 0x01;
    packet.header.tc = tmp8 >> 1 & 0x01;
    packet.header.rd = tmp8 >> 0 & 0x01;
    /* RA+padding(3)+RCODE */
    data = _read8(data, &tmp8);
    packet.header.ra = tmp8 >> 7 & 0x01;
    packet.header.rcode = (DNSPacketRC)(tmp8 & 0xF);
    /* Counts */
    data = _read16(data, &packet.header.questionCount);
    data = _read16(data, &packet.header.answerCount);
    data = _read16(data, &packet.header.authorityCount);
    data = _read16(data, &packet.header.additionalCount);
    /* Questions */
    packet.questions = (DNSQuestion *)malloc(sizeof(DNSQuestion) * packet.header.questionCount);
    for (int i = 0; i < packet.header.questionCount; i++) {
        DNSQuestion *r = &packet.questions[i];
        r->name = (char *)malloc(sizeof(char) * 256);
        data += decodeQname((char *)data, buffer.data, r->name);
        uint16_t tmp;
        data = _read16(data, &tmp);
        r->qtype = tmp;
        data = _read16(data, &tmp);
        r->qclass = tmp;
    }
    /* Answers */
    if (packet.header.answerCount > 0) {
        packet.answers = (DNSRecord *)malloc(sizeof(DNSRecord) * packet.header.answerCount);
        for (int i = 0; i < packet.header.answerCount; i++) {
            DNSRecord *r = &packet.answers[i];
            r->name = (char *)malloc(sizeof(char) * 256);
            data += decodeQname((char *)data, buffer.data, r->name);
            uint16_t tmp;
            data = _read16(data, &tmp);
            r->type = tmp;
            data = _read16(data, &tmp);
            r->rclass = tmp;
            data = _read32(data, &r->ttl);
            data = _read16(data, &r->rdataLength);
            r->rdata = (char *)malloc(sizeof(char) * r->rdataLength);
            memcpy(r->rdata, data, r->rdataLength);
        }
    } else {
        packet.answers = NULL;
    }
    /* Authorities */
    if (packet.header.authorityCount > 0) {
        packet.authorities = (DNSRecord *)malloc(sizeof(DNSRecord) * packet.header.authorityCount);
        for (int i = 0; i < packet.header.authorityCount; i++) {
            DNSRecord *r = &packet.authorities[i];
            r->name = (char *)malloc(sizeof(char) * 256);
            data += decodeQname((char *)data, buffer.data, r->name);
            uint16_t tmp;
            data = _read16(data, &tmp);
            r->type = tmp;
            data = _read16(data, &tmp);
            r->rclass = tmp;
            data = _read32(data, &r->ttl);
            data = _read16(data, &r->rdataLength);
            r->rdata = (char *)malloc(sizeof(char) * r->rdataLength);
            memcpy(r->rdata, data, r->rdataLength);
        }
    } else {
        packet.authorities = NULL;
    }
    /* Additional */
    if (packet.header.additionalCount > 0) {
        packet.additional = (DNSRecord *)malloc(sizeof(DNSRecord) * packet.header.additionalCount);
        for (int i = 0; i < packet.header.additionalCount; i++) {
            DNSRecord *r = &packet.additional[i];
            r->name = (char *)malloc(sizeof(char) * 256);
            data += decodeQname((char *)data, buffer.data, r->name);
            uint16_t tmp;
            data = _read16(data, &tmp);
            r->type = tmp;
            data = _read16(data, &tmp);
            r->rclass = tmp;
            data = _read32(data, &r->ttl);
            data = _read16(data, &r->rdataLength);
            r->rdata = (char *)malloc(sizeof(char) * r->rdataLength);
            memcpy(r->rdata, data, r->rdataLength);
        }
    } else {
        packet.additional = NULL;
    }
    return packet;
}
void DNSPacket_destroy(DNSPacket packet) {
    for (int i = 0; i < packet.header.questionCount; i++) {
        free(packet.questions[i].name);
        packet.questions[i].name = NULL;
    }
    for (int i = 0; i < packet.header.answerCount; i++) {
        free(packet.answers[i].name);
        free(packet.answers[i].rdata);
        packet.answers[i].name = NULL;
        packet.answers[i].rdata = NULL;
    }
    for (int i = 0; i < packet.header.authorityCount; i++) {
        free(packet.authorities[i].name);
        free(packet.authorities[i].rdata);
        packet.authorities[i].name = NULL;
        packet.authorities[i].rdata = NULL;
    }
    for (int i = 0; i < packet.header.additionalCount; i++) {
        free(packet.additional[i].name);
        free(packet.additional[i].rdata);
        packet.additional[i].name = NULL;
        packet.additional[i].rdata = NULL;
    }
    free(packet.answers);
    free(packet.questions);
    free(packet.authorities);
    free(packet.additional);
    packet.answers = NULL;
    packet.questions = NULL;
    packet.authorities = NULL;
    packet.additional = NULL;
    packet.header.questionCount = 0;
    packet.header.answerCount = 0;
    packet.header.authorityCount = 0;
    packet.header.additionalCount = 0;
}
void DNSPacket_fillQuery(DNSPacket *packet) {
    packet->header.qr = QRQUERY;
    packet->header.opcode = QUERY;
    packet->header.rcode = OK;
    packet->header.aa = 0;
    packet->header.tc = 0;
    packet->header.rd = 1;
    packet->header.ra = 0;
    packet->header.questionCount = 1;
    packet->header.answerCount = 0;
    packet->header.authorityCount = 0;
    packet->header.additionalCount = 0;
    packet->answers = NULL;
    packet->authorities = NULL;
    packet->additional = NULL;
}