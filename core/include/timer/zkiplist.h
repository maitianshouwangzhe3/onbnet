#pragma once

#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/2 */

struct zskiplist_node;
struct zskiplistLevel {
        zskiplist_node *forward;
};

struct zskiplist_node {
    unsigned long long score; // 时间戳
    zskiplistLevel* level;
};

typedef struct zskiplist {
    struct zskiplist_node *header;
    int length;
    int level;
} zskiplist;

zskiplist* zsl_create();
void       zsl_free(zskiplist* zsl);

bool zsl_insert(zskiplist* zsl, zskiplist_node* node);
zskiplist_node* zsl_min(zskiplist *zsl);

void zsl_delete_head(zskiplist* zsl);
void zsl_delete(zskiplist* zsl, zskiplist_node* zn); 

int zsl_random_level(void);
zskiplist_node* zsl_create_node(int level, unsigned long score);
void zsl_delete_node(zskiplist_node* node);
void* zsl_create_level(int level);