
#include "zkiplist.h"

#include <cstdlib>

int zslRandomLevel(void) {
    int level = 1;
    while ((rand()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))

        level += 1;
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

static void zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update) {
    int i;
    for (i = 0; i < zsl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].forward = x->level[i].forward;
        }
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}

zskiplistNode *zslCreateNode(int level, unsigned long score) {
    zskiplistNode* zn = (zskiplistNode*)malloc(sizeof(zskiplistNode) + level * sizeof(zskiplistLevel));
    zn->level = (zskiplistLevel*)malloc(level * sizeof(zskiplistLevel));
    zn->score = score;
    return zn;
}

zskiplist *zslCreate(void) {
    int j = 0;
    zskiplist* zsl = nullptr;

    zsl = (zskiplist*)malloc(sizeof(zskiplist));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        zsl->header->level[j].forward = NULL;
    }
    return zsl;
}

void zslFree(zskiplist *zsl) {
    zskiplistNode *node = zsl->header->level[0].forward, *next;
    free(zsl->header->level);
    free(zsl->header);
    while(node) {
        next = node->level[0].forward;
        free(node);
        node = next;
    }
    free(zsl);
}

bool zslInsert(zskiplist *zsl, zskiplistNode* node) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    int i, level;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
                x->level[i].forward->score < node->score)
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    level = zslRandomLevel();

    if (level > zsl->level) {
        for (i = zsl->level; i < level; i++) {
            update[i] = zsl->header;
        }
        zsl->level = level;
    }

    x = node;
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = node;
    }

    zsl->length++;
    return true;
}

zskiplistNode* zslMin(zskiplist *zsl) {
    zskiplistNode *x;
    x = zsl->header;
    return x->level[0].forward;
}

void zslDeleteHead(zskiplist* zsl) {
    zskiplistNode *x = zslMin(zsl);
    if (!x) return;
    int i;
    for (i = zsl->level-1; i >= 0; i--) {
        if (zsl->header->level[i].forward == x) {
            zsl->header->level[i].forward = x->level[i].forward;
        }
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}

void zslDelete(zskiplist* zsl, zskiplistNode* zn) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    int i;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
                x->level[i].forward->score < zn->score)
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    x = x->level[0].forward;
    if (x && zn->score == x->score) {
        zslDeleteNode(zsl, x, update);
    }
}

void zslDeleteNode(zskiplistNode* node) {
    if(node) {
        free(node->level);
    }
}

void* zslCreateLevel(int level) {
    return malloc(level * sizeof(zskiplistLevel));
}