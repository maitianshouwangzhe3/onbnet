
#include "minheap.h"

#include <cstdlib>

#define min_heap_elem_greater(a, b) \
    ((a)->time > (b)->time)

static void min_heap_shift_up_(min_heap* s, uint32_t hole_index, timer_entry* e) {
    //计算父节点索引
    uint32_t parent = (hole_index - 1) / 2;
    //当前插入位置的索引不为0，并且插入位置的父节点的值比当前插入的值大，则交换位置知道这个条件不满足就把插入的元素插入到当前位置
    while (hole_index && min_heap_elem_greater(s->p[parent], e))
    {
        (s->p[hole_index] = s->p[parent])->min_heap_idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->min_heap_idx = hole_index;
}

static void min_heap_shift_down_(min_heap* s, uint32_t hole_index, timer_entry* e)
{
    // 计算右子节点的索引
    uint32_t min_child = 2 * (hole_index + 1);
    // 如果右子节点存在则继续向下调整
    while (min_child <= s->n)
    {
        // 确保指向最小的子节点
        min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
        // 如果最小的子节点还是小于尾节点则不调整直接交换，否则继续向下寻找
        if (!(min_heap_elem_greater(e, s->p[min_child]))) {
            break;
        }
            
        (s->p[hole_index] = s->p[min_child])->min_heap_idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
    }
    (s->p[hole_index] = e)->min_heap_idx = hole_index;
}

// 堆大小动态增长
static int min_heap_reserve(min_heap* s, uint32_t n) {
    if (s->a < n)
    {
        timer_entry** p;
        unsigned a = s->a ? s->a * 2 : 8;
        if (a < n)
            a = n;
        if (!(p = (timer_entry**)realloc(s->p, a * sizeof(*p)))) {
            return -1;
        }
            
        s->p = p;
        s->a = a;
    }
    return 0;
}

int min_heap_init(min_heap** h, uint32_t a) {

    *h = (min_heap*)malloc(sizeof(min_heap));
    if(!*h) {
        return -1;
    }

    (*h)->p = (timer_entry**)malloc(a * sizeof(*((*h)->p)));
    if(!(*h)->p) {
        return -1;
    }

    (*h)->a = a;
    (*h)->n = 0;
    return 0;
}

int min_heap_free(min_heap* h) {
    if(h) {
        return -1;
    }

    if(h->p) {
        free(h->p);
    }

    free(h);
    return 0;
}

int min_heap_push(min_heap* h, timer_entry* x) {
    if(min_heap_reserve(h, h->n + 1)) {
        return -1;
    }

    min_heap_shift_up_(h, h->n++, x);
    return 0;
}

int min_heap_pop(min_heap* h, timer_entry** x) {
    if (h->n)
    {
        timer_entry* e = *(h->p);
        min_heap_shift_down_(h, 0, h->p[--h->n]);
        e->min_heap_idx = -1;
        *x = e;
        return 0;
    }
    return -1;
}

int min_heap_top(min_heap* h, timer_entry** x) {
    if (h->n)
    {
        timer_entry* e = *(h->p);
        *x = e;
        return 0;
    }
    return -1;
}