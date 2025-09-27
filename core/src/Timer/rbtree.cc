
#include "rbtree.h"


static void insert(rbtree_node* r, rbtree_node* n, rbtree_node* s) {
    rbtree_node  **p = nullptr;

    for ( ;; ) {
        p = (n->key < r->key) ? &r->left : &r->right;

        if (*p == s) {
            break;
        }

        r = *p;
    }

    *p = n;
    n->parent = r;
    n->left = s;
    n->right = s;
    n->color = 1;
}

static void rbtree_left_rotate(rbtree_node**r, rbtree_node* s, rbtree_node* n) {
    rbtree_node* temp = n->right;
    n->right = n->right->left;

    if(temp->left != s) {
        temp->left->parent = n;
    }

    temp->parent = n->parent;

    if(n == *r) {
        *r = temp;
    } else if(n == n->parent->left) {
        n->parent->left = temp;
    } else {
        n->parent->right = temp;
    }

    temp->left = n;
    n->parent = temp;
}

static void rbtree_right_rotate(rbtree_node**r, rbtree_node* s, rbtree_node* n) {
    rbtree_node  *temp;

    temp = n->left;
    n->left = temp->right;

    if (temp->right != s) {
        temp->right->parent = n;
    }

    temp->parent = n->parent;

    if (n == *r) {
        *r = temp;

    } else if (n == n->parent->right) {
        n->parent->right = temp;

    } else {
        n->parent->left = temp;
    }

    temp->right = n;
    n->parent = temp;
}

static rbtree_node* rbtree_min(rbtree_node* n, rbtree_node* s) {
    if(!n) {
        return nullptr;
    }

    while(n->left != s) {
        n = n->left;
    }

    return n;
}

static rbtree_node* rbtree_max(rbtree_node* n, rbtree_node* s) {
    if(!n) {
        return nullptr;
    }

    while(n->right != s) {
        n = n->right;
    }

    return n;
}

void rbtree_init(rbtree* r, rbtree_node* s) {
    s->color = 0;
    r->root = s;
    r->sentinel = s;
}

void rbtree_insert(rbtree *tree, rbtree_node *node) {
    rbtree_node** root = &tree->root;
    rbtree_node* sentinel = tree->sentinel;

    if(*root == sentinel) {
        node->parent = nullptr;
        node->left = sentinel;
        node->right = sentinel;
        node->color = 0;
        *root = node;
        return;
    }

    // 插入节点
    insert(*root, node, sentinel);

    rbtree_node* temp = nullptr;
    //判断是否需要调整使其平衡
    while(node != *root && node->parent->color) {
        if(node->parent == node->parent->parent->left) {
            temp = node->parent->parent->right;
            //如果叔父节点是红色则直接变色并进入下一轮调整
            if(temp->color) {
                node->parent->color = 0;
                temp->color = 0;
                node->parent->parent->color = 1;
                node = node->parent->parent;
            } else {
                //如果当前插入节点是父节点的左节点先对父节点进行左旋    
                if(node == node->parent->right) {
                    node = node->parent;
                    rbtree_left_rotate(root, sentinel, node);
                }

                node->parent->color = 0;
                node->parent->parent->color = 1;
                rbtree_right_rotate(root, sentinel, node->parent->parent);
            }
        } else {
            temp = node->parent->parent->left;
            //如果叔父节点是红色则直接变色并进入下一轮调整
            if(temp->color) {
                node->parent->color = 0;
                temp->color = 0;
                node->parent->parent->color = 1;
                node = node->parent->parent;
            } else {
                //如果当前插入节点是父节点的左节点先对父节点进行右旋
                if(node == node->parent->left) {
                    node = node->parent;
                    rbtree_right_rotate(root, sentinel, node);
                }

                node->parent->color = 0;
                node->parent->parent->color = 1;
                rbtree_left_rotate(root, sentinel, node->parent->parent);
            }
        }
    }

    (*root)->color = 0;
}

void rbtree_delete(rbtree *tree, rbtree_node *node) {
    rbtree_node** root = &tree->root;
    rbtree_node* sentinel = tree->sentinel;
    rbtree_node* subst = nullptr;
    rbtree_node* temp = nullptr;

    if (node->left == sentinel) {
        temp = node->right;
        subst = node;

    } else if (node->right == sentinel) {
        temp = node->left;
        subst = node;
    } else {
        subst = rbtree_min(node->right, sentinel);
        if (subst->left != sentinel) {
            temp = subst->left;
        } else {
            temp = subst->right;
        }
    }

    if (subst == *root) {
        *root = temp;
        temp->color = 0;
        return;
    }

    if (subst == subst->parent->left) {
        subst->parent->left = temp;
    } else {
        subst->parent->right = temp;
    }

    if (subst == node) {
        temp->parent = subst->parent;
    } else {
        if (subst->parent == node) {
            temp->parent = subst;

        } else {
            temp->parent = subst->parent;
        }

        subst->left = node->left;
        subst->right = node->right;
        subst->parent = node->parent;
        subst->color = node->color;

        if (node == *root) {
            *root = subst;
        } else {
            if (node == node->parent->left) {
                node->parent->left = subst;
            } else {
                node->parent->right = subst;
            }
        }

        if (subst->left != sentinel) {
            subst->left->parent = subst;
        }

        if (subst->right != sentinel) {
            subst->right->parent = subst;
        }
    }

    if (subst->color) {
        return;
    }

    rbtree_node* w = nullptr;
    while (temp != *root && !temp->color) {

        if (temp == temp->parent->left) {
            w = temp->parent->right;

            if (w->color) {
                w->color = 1;
                temp->parent->color = 0;
                rbtree_left_rotate(root, sentinel, temp->parent);
                w = temp->parent->right;
            }

            if (!w->left->color && !w->right->color) {
                w->color = 0;
                temp = temp->parent;

            } else {
                if (!w->right->color) {
                    w->left->color = 0;
                    w->color = 1;
                    rbtree_right_rotate(root, sentinel, w);
                    w = temp->parent->right;
                }

                w->color = temp->color;
                temp->parent->color = 0;
                w->right->color = 0;
                rbtree_left_rotate(root, sentinel, temp->parent);
                temp = *root;
            }

        } else {
            w = temp->parent->left;

            if (w->color) {
                w->color = 0;
                temp->parent->color = 1;
                rbtree_right_rotate(root, sentinel, temp->parent);
                w = temp->parent->left;
            }

            if (!w->left->color && !w->right->color) {
                w->color = 1;
                temp = temp->parent;

            } else {
                if (!w->left->color) {
                    w->right->color = 0;
                    w->color = 1;
                    rbtree_left_rotate(root, sentinel, w);
                    w = temp->parent->left;
                }

                w->color = temp->parent->color;
                temp->parent = 0;
                w->left->color = 0;
                rbtree_right_rotate(root, sentinel, temp->parent);
                temp = *root;
            }
        }
    }

    temp->color = 0;
}

rbtree_node* rbtree_min(rbtree *tree) {
    if(!tree->root || tree->root == tree->sentinel) {
        return nullptr;
    }

    return rbtree_min(tree->root, tree->sentinel);
}

rbtree_node* rbtree_max(rbtree *tree) {
    if(!tree->root || tree->root == tree->sentinel) {
        return nullptr;
    }

    return rbtree_max(tree->root, tree->sentinel);
}

rbtree_node* ngx_rbtree_next(rbtree *tree, rbtree_node *node)
{
    rbtree_node* root = nullptr;
    rbtree_node* sentinel = nullptr;
    rbtree_node* parent = nullptr;

    sentinel = tree->sentinel;

    if (node->right != sentinel) {
        return rbtree_min(node->right, sentinel);
    }

    root = tree->root;

    for ( ;; ) {
        parent = node->parent;

        if (node == root) {
            return nullptr;
        }

        if (node == parent->left) {
            return parent;
        }

        node = parent;
    }
}