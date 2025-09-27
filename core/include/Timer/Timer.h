#pragma once


#include "minheap.h"
#include "rbtree.h"
#include "zkiplist.h"
#include "timewheel.h"
#include <cstdint>
#include <functional>
#include <ctime>

enum TimeType {
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

using CallBack = std::function<void(void)>;

struct TimeNode {
    timer_entry env;
    CallBack cb;
};

struct TimeNodeRb {
    rbtree_node env;
    CallBack cb;
};

struct TimeNodeZp {
    zskiplistNode env; 
    CallBack cb;   
};

struct TimeNodeTw {
    timer_node env;
    CallBack cb;
};

class Timer {
public:
    Timer(uint32_t size);
    Timer(TimeType Type, uint32_t size = 0);
    Timer() = delete;
    ~Timer();

    int addTimer(uint64_t time, CallBack cb);

    void run();

    void Start();

    void stop();

    void operator()();

    static Timer* inst;

private:
    uint64_t GetNowTime();

    void AddMinHeapTimer(uint64_t time, CallBack cb);

    void AddRbtreeTimer(uint64_t time, CallBack cb);

    void AddZskiplistTimer(uint64_t time, CallBack cb);

    void AddTimeWheelTimer(uint64_t time, CallBack cb);

    void updateMinHeapTimer(uint64_t& sleep, uint64_t& now);

    void updateRbtreeTimer(uint64_t& sleep, uint64_t& now);

    void updateZskiplistTimer(uint64_t& sleep, uint64_t& now);

    void updateTimeWheelTimer(uint64_t& sleep, uint64_t& now);
private:
    min_heap* _heap;
    rbtree*   _rbtree;
    bool _close;
    TimeType _Type;
    zskiplist* _zskiplist;
    timer* _timewheel;
};

#define TimerInst Timer::inst