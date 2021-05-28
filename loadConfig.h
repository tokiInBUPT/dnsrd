#include "DNSPacket.h"
#include "RBTree.h"

#define MAX_BUF_LEN 1024

void loadConfig(char *fname, struct Node **rbTree);