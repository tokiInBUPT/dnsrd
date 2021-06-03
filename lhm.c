#include "lhm.h"
/*判断是否为key值是否相同*/
int same_key(Key key1, Key key2) {
    if (key1.qtype != key2.qtype || strcmp(key1.name, key2.name)) {
        return 0;
    }
    return 1;
}

struct hash *HashMap(struct hash *table, Key key, int capacity) { //哈希地址
    unsigned long long hashValue = 0;
    hashValue += key.qtype;
    for (int i = 0; i < strlen(key.name); i++) {
        hashValue *= 31;
        hashValue += key.name[i];
    }
    int addr = hashValue % capacity; //求余数
    struct hash *tmptmp = &table[addr];
    return &table[addr];
}

void HeadInsertion(struct node *head, struct node *cur) { //双链表头插法
    if (cur->prev == NULL && cur->next == NULL) {         // cur 不在链表中
        cur->prev = head;
        cur->next = head->next;
        head->next->prev = cur;
        head->next = cur;
    } else {                             // cur 在链表中
        struct node *fisrt = head->next; //链表的第一个数据结点
        if (fisrt != cur) {              //cur 是否已在第一个
            cur->prev->next = cur->next; //改变前驱结点指向
            cur->next->prev = cur->prev; //改变后继结点指向
            cur->next = fisrt;           //插入到第一个结点位置
            cur->prev = head;
            head->next = cur;
            fisrt->prev = cur;
        }
    }
}

LRUCache *lRUCacheCreate(int capacity) {
    if (capacity <= 0) { //传参检查
        return NULL;
    }
    LRUCache *obj = (LRUCache *)malloc(sizeof(LRUCache));
    obj->table = (struct hash *)malloc(capacity * sizeof(struct hash));
    memset(obj->table, 0, capacity * sizeof(struct hash));
    obj->head = (struct node *)malloc(sizeof(struct node));
    obj->tail = (struct node *)malloc(sizeof(struct node));
    //创建头、尾结点并初始化
    obj->head->prev = NULL;
    obj->head->next = obj->tail;
    obj->tail->prev = obj->head;
    obj->tail->next = NULL;
    //初始化缓存 大小 和 容量
    obj->size = 0;
    obj->capacity = capacity;
    return obj;
}

MyData lRUCacheGet(LRUCache *obj, Key key) {
    struct hash *addr = HashMap(obj->table, key, obj->capacity); //取得哈希地址
    addr = addr->next;                                           //跳过头结点
    if (addr == NULL) {
        MyData tmp;
        tmp.answerCount = Nothingness;
        return tmp;
    }
    while (addr->next != NULL && !same_key(addr->unused->key, key)) { //寻找密钥是否存在
        addr = addr->next;
    }
    if (same_key(addr->unused->key, key)) {     //查找成功
        HeadInsertion(obj->head, addr->unused); //更新至表头
        return addr->unused->value;
    }
    MyData tmp;
    tmp.answerCount = Nothingness;
    return tmp;
}

void lRUCachePut(LRUCache *obj, Key key, MyData value) {
    struct hash *addr = HashMap(obj->table, key, obj->capacity);                                        //取得哈希地址
    if (lRUCacheGet(obj, key).answerCount == Nothingness) {                                             //key所指向的表项不存在或已过期
        if (obj->size >= obj->capacity) {                                                               //缓存容量达到上限
                                                                                                        //start
            struct node *last;                                                                          //最后一个数据结点
            for (last = obj->tail->prev; last != obj->head && last->value.time == 0; last = last->prev) //找到最后一个可删除结点（即保证不删除host文件中的结点）
            {
                printf("name: %s ", last->key.name);
                printf("time: %d\n", last->value.time);
            }
            if (last == obj->head) {

                printf("!!!No room for values outside the hosts file\n");
                return;
            }
            //end
            struct hash *remove = HashMap(obj->table, last->key, obj->capacity); //舍弃结点的哈希地址
            struct hash *ptr = remove;
            remove = remove->next;                                             //跳过头结点
            while (remove->next && same_key(remove->unused->key, last->key)) { //找到最久未使用结点的前一个结点
                ptr = remove;
                remove = remove->next;
            }
            ptr->next = remove->next; //在hash table[last->key % capacity] 链表中删除最久未使用的hash结点
            remove->next = NULL;
            remove->unused = NULL; //解除映射
            free(remove);          //回收hash结点资源
            struct hash *new_node = (struct hash *)malloc(sizeof(struct hash));
            new_node->next = addr->next; //连接到 table[key % capacity] 的链表中
            addr->next = new_node;
            new_node->unused = last; //最大化利用双链表中的结点，对其重映射(节约空间)
            last->key = key;         //重新赋值
            last->value = value;
            HeadInsertion(obj->head, last); //更新最近使用的数据
        } else {                            //缓存未达上限
            //创建(密钥\数据)结点,并建立映射
            struct hash *new_node = (struct hash *)malloc(sizeof(struct hash));
            new_node->unused = (struct node *)malloc(sizeof(struct node));
            new_node->next = addr->next; //连接到 table[key % capacity] 的链表中
            addr->next = new_node;
            new_node->unused->prev = NULL; //标记该结点是新创建的,不在双向链表中
            new_node->unused->next = NULL;
            new_node->unused->key = key;                //插入密钥
            new_node->unused->value = value;            //插入数据
            HeadInsertion(obj->head, new_node->unused); //更新最近使用的数据
            ++(obj->size);                              //缓存大小+1
        }
    } else {                            //密钥已存在
                                        // lRUCacheGet 函数已经更新双链表表头，故此处不用更新
        obj->head->next->value = value; //替换数据值
    }
}

void lRUCacheFree(LRUCache *obj) {
    free(obj->table);
    struct node *tmp = obj->head;
    obj->head = obj->head->next;
    free(tmp);
    while (obj->head != obj->tail) {
        while (obj->head->value.answerCount--) {
            if (obj->head->value.answers[obj->head->value.answerCount].rdata != NULL) {
                free(obj->head->value.answers[obj->head->value.answerCount].rdata);
            }
        }
        free(obj->head->value.answers);
        struct node *tmp = obj->head;
        obj->head = obj->head->next;
        free(tmp);
    }
    free(obj->tail);
    free(obj);
}