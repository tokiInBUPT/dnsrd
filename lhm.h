#ifndef LHM_H
#define LHM_H
#define Nothingness 0
#include "DNSPacket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Key {
    DNSQType qtype;
    char name[255];
} Key;

typedef struct MyData {
    uint32_t time;
    uint32_t answerCount;
    DNSRecord *answers;
} MyData;

struct node {
    Key key;
    MyData value;
    struct node *prev;
    struct node *next;
}; //双向链表

struct hash {
    struct node *unused; //数据的未使用时长
    struct hash *next;   //拉链法解决哈希冲突
};                       //哈希表结构

typedef struct {
    int size;           //当前缓存大小
    int capacity;       //缓存容量
    struct hash *table; //哈希表
    //维护一个双向链表用于记录 数据的未使用时长
    struct node *head; //后继 指向 最近使用的数据
    struct node *tail; //前驱 指向 最久未使用的数据
} LRUCache;

/*
 * 创建LRU缓存
 */
LRUCache *lRUCacheCreate(int capacity);
/*
 * 查询LRU缓存
 */
MyData lRUCacheGet(LRUCache *obj, Key key);
/*
 * 插入LRU缓存
 */
void lRUCachePut(LRUCache *obj, Key key, MyData value);
/*
 * 释放LRU缓存
 */
void lRUCacheFree(LRUCache *obj);
#endif