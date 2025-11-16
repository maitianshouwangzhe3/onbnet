#pragma once

struct rbtree_node {
    unsigned long long key;
    rbtree_node*       left;
    rbtree_node*       right;
    rbtree_node*       parent;
    unsigned char      color; // 0: red, 1: black
};

struct rbtree {
    rbtree_node* root;
    rbtree_node* sentinel; //哨兵节点
};

void rbtree_init(rbtree* r, rbtree_node* s);

void rbtree_insert(rbtree *tree, rbtree_node *node);

void rbtree_delete(rbtree *tree, rbtree_node *node);

rbtree_node* rbtree_min(rbtree *tree);

rbtree_node* rbtree_max(rbtree *tree);