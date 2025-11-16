#pragma once


#include "minheap.h"
#include "rbtree.h"
#include "zkiplist.h"
#include "timewheel.h"
#include <cstdint>
#include <functional>
#include <ctime>

namespace onbnet::tw {
enum time_type {
    MIN_HEAP,
    RBTREE,
    ZKIPLIST,
    TIMEWHEEL,
};

extern "C"{
#define offsetofs(s,m) (size_t)(&reinterpret_cast<const volatile char&>((((s*)0)->m)))
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetofs(type, member) );})
}

using callback = std::function<void(void)>;

struct time_node {
    timer_entry env;
    callback cb;
};

struct time_node_rb {
    rbtree_node env;
    callback cb;
};

struct time_node_zp {
    zskiplist_node env; 
    callback cb;   
};

struct time_node_tw {
    timer_node env;
    callback cb;
};

class timer {
public:
    timer(uint32_t size);
    timer(time_type Type, uint32_t size = 0);
    timer() = delete;
    ~timer();

    int add_timer(uint64_t time, callback cb);

    void run();

    void start();

    void stop();

    void operator()();

    static timer* inst;

private:
    uint64_t get_now_time();

    void add_min_heap_timer(uint64_t time, callback cb);

    void add_rbtree_timer(uint64_t time, callback cb);

    void add_zskiplist_timer(uint64_t time, callback cb);

    void add_time_wheel_timer(uint64_t time, callback cb);

    void update_min_heap_timer(uint64_t& sleep, uint64_t& now);

    void update_rbtree_timer(uint64_t& sleep, uint64_t& now);

    void update_zskiplist_timer(uint64_t& sleep, uint64_t& now);

    void update_time_wheel_timer(uint64_t& sleep, uint64_t& now);
private:
    min_heap* _heap;
    rbtree*   _rbtree;
    bool _close;
    time_type _Type;
    zskiplist* _zskiplist;
    timer_wheel* _timewheel;
};

};

#define timer_inst onbnet::tw::timer::inst