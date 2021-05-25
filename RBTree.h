#include "DNSPacket.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum type
{
    RED,
    BLACK
};

typedef struct MyData
{
    uint32_t time;
    DNSRecord record;
} MyData;

typedef struct Key
{
    DNSQType qtype;
    char name[255];
} Key;

struct Node
{
    Key key;
    struct MyData myData;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
    enum type color;
};

struct Node *initRBTree();
struct Node *RB_insert(struct Node *T, Key key, MyData mydata);
struct Node *BST_search(struct Node *root, Key key);
struct Node *RB_delete(struct Node *T, struct Node *z);
void deleteAll(struct Node *root);