#pragma once

#include <cstdint>

struct timer_entry {
    uint64_t time;            // 时间戳，用来表示定时任务的过期时间
    uint32_t min_heap_idx;    // 当前节点在堆(数组)中的索引
};

struct min_heap {
    timer_entry** p;
    uint32_t n; // n 为实际元素个数  
    uint32_t a; // a 为容量
};


int min_heap_init(min_heap** h, uint32_t a);

int min_heap_free(min_heap* h);

int min_heap_push(min_heap* h, timer_entry* x);

int min_heap_pop(min_heap* h, timer_entry** x);

int min_heap_top(min_heap* h, timer_entry** x);