
#include "zkiplist.h"

#include <cstdlib>

int zsl_random_level(void) {
    int level = 1;
    while ((rand()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))

        level += 1;
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

static void zsl_delete_node(zskiplist *zsl, zskiplist_node *x, zskiplist_node **update) {
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

zskiplist_node *zsl_create_node(int level, unsigned long score) {
    zskiplist_node* zn = (zskiplist_node*)malloc(sizeof(zskiplist_node) + level * sizeof(zskiplistLevel));
    zn->level = (zskiplistLevel*)malloc(level * sizeof(zskiplistLevel));
    zn->score = score;
    return zn;
}

zskiplist *zsl_create(void) {
    int j = 0;
    zskiplist* zsl = nullptr;

    zsl = (zskiplist*)malloc(sizeof(zskiplist));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zsl_create_node(ZSKIPLIST_MAXLEVEL,0);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        zsl->header->level[j].forward = NULL;
    }
    return zsl;
}

void zsl_free(zskiplist *zsl) {
    zskiplist_node *node = zsl->header->level[0].forward, *next;
    free(zsl->header->level);
    free(zsl->header);
    while(node) {
        next = node->level[0].forward;
        free(node);
        node = next;
    }
    free(zsl);
}

bool zsl_insert(zskiplist *zsl, zskiplist_node* node) {
    zskiplist_node *update[ZSKIPLIST_MAXLEVEL], *x;
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
    level = zsl_random_level();

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

zskiplist_node* zsl_min(zskiplist *zsl) {
    zskiplist_node *x;
    x = zsl->header;
    return x->level[0].forward;
}

void zsl_delete_head(zskiplist* zsl) {
    zskiplist_node *x = zsl_min(zsl);
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

void zslDelete(zskiplist* zsl, zskiplist_node* zn) {
    zskiplist_node *update[ZSKIPLIST_MAXLEVEL], *x;
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
        zsl_delete_node(zsl, x, update);
    }
}

void zsl_delete_node(zskiplist_node* node) {
    if(node) {
        free(node->level);
    }
}

void* zsl_create_level(int level) {
    return malloc(level * sizeof(zskiplistLevel));
}