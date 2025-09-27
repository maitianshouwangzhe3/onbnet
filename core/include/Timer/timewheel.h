#pragma once
#include "spinlock.h"
#include <cstdint>

#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)

struct timer_node {
	struct timer_node *next;
	uint32_t expire;
};

struct link_list {
	struct timer_node head;
	struct timer_node *tail;
};

struct timer {
	struct link_list* near;
	struct link_list** t;
	struct spinlock lock;
	uint32_t time;
	uint32_t starttime;
	uint64_t current;
	uint64_t current_point;
};

// struct timeWheelNode {
//     unsigned long long score;
//     timer* level;
// };

int onbnet_timeout(uint32_t handle, int time, int session);
void onbnet_updatetime(void);
uint32_t onbnet_starttime(void);
uint64_t onbnet_thread_time(void);
void timer_add(struct timer *T, struct timer_node* node, uint32_t time);
// void onbnet_timer_init(void);

struct timer_node* link_clear(struct link_list *list);
void timer_shift(struct timer *T);
timer* onbnet_timer_create();

