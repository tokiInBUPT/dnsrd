#include "DNSPacket.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <winsock2.h>

enum type
{
    RED,
    BLACK
};

typedef struct MyData
{
    uint16_t id;
    sockaddr_in addr;
    uint16_t time;
} MyData;


struct Node_int
{
    uint16_t key;
    uint16_t myData;
    struct Node_int *left;
    struct Node_int *right;
    struct Node_int *parent;
    enum type color;
};

struct Node_int *initRBTree_int();
struct Node_int *RB_insert_int(struct Node_int *T, uint16_t key, uint16_t mydata);
struct Node_int *BST_search_int(struct Node_int *root, uint16_t key);
struct Node_int *RB_delete_int(struct Node_int *T, struct Node_int *z);
void deleteAll_int(struct Node_int *root);