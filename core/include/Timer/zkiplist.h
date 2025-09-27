#pragma once

#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/2 */

struct zskiplistNode;
struct zskiplistLevel {
        zskiplistNode *forward;
};

struct zskiplistNode {
    unsigned long long score; // 时间戳
    zskiplistLevel* level;
};

typedef struct zskiplist {
    struct zskiplistNode *header;
    int length;
    int level;
} zskiplist;

zskiplist* zslCreate();
void       zslFree(zskiplist* zsl);

bool zslInsert(zskiplist* zsl, zskiplistNode* node);
zskiplistNode* zslMin(zskiplist *zsl);

void zslDeleteHead(zskiplist* zsl);
void zslDelete(zskiplist* zsl, zskiplistNode* zn); 

int zslRandomLevel(void);
zskiplistNode* zslCreateNode(int level, unsigned long score);
void zslDeleteNode(zskiplistNode* node);
void* zslCreateLevel(int level);