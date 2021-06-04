#ifndef LHM_H
#define LHM_H
#define Nothingness 0
#include "DNSPacket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*缓存数据的关键字*/
typedef struct Key {
    DNSQType qtype; //查询类型
    char name[256]; //待查询字符串
} Key;

/*缓存的数据*/
typedef struct MyData {
    time_t time;          //缓存时间
    uint32_t answerCount; //资源信息的数量
    DNSRecord *answers;   //改数据项中的资源信息（例如IP地址、域名等）
} MyData;

/*LRU中的结点*/
struct node {
    Key key;           //关键码
    MyData value;      //结点中的数据
    struct node *prev; //前指针
    struct node *next; //后指针
};                     //双向链表

struct hash {
    struct node *recordNode; //数据的未使用时长
    struct hash *next;       //拉链法解决哈希冲突
};                           //哈希表结构

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