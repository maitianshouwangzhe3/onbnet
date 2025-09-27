
#include "Timer.h"

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <thread>

Timer* Timer::inst = nullptr;

Timer::Timer(uint32_t size) {
    if (!inst) {
        Timer::inst = this;
    } else {
        throw std::runtime_error("Timer::inst is not null");
    }

    min_heap_init(&_heap, size);
    _close = false;
}

Timer::Timer(TimeType Type, uint32_t size) {
    if (!inst) {
        Timer::inst = this;
    } else {
        throw std::runtime_error("Timer::inst is not null");
    }

    switch (Type) {
    case  MIN_HEAP:
        min_heap_init(&_heap, size);
        break;
    case RBTREE:
        _rbtree = new rbtree();
        if(_rbtree) {
            _rbtree->sentinel = new rbtree_node();
            rbtree_init(_rbtree, _rbtree->sentinel);
        }
        break;
    case ZKIPLIST:
        _zskiplist = zslCreate();
        break;
    case TIMEWHEEL:
        _timewheel = onbnet_timer_create();
        break;
    }
    _close = false;
    _Type = Type;
}

Timer::~Timer() {
    switch (_Type) {
        case MIN_HEAP:
            min_heap_free(_heap);
        break;
        case RBTREE:
            delete _rbtree->sentinel;
            delete _rbtree;      
        break;
        case ZKIPLIST:
            zslFree(_zskiplist);
        break;
        case TIMEWHEEL:
        break;
    }
}

int Timer::addTimer(uint64_t time, CallBack cb) {
    switch (_Type) {
    case MIN_HEAP:
        AddMinHeapTimer(time, cb);
        break;
    case RBTREE:
        AddRbtreeTimer(time, cb);
        break;
    case ZKIPLIST:
        AddZskiplistTimer(time, cb);
        break;
    case TIMEWHEEL:
        AddTimeWheelTimer(time, cb);
        break;
    }
    return 0;
}

uint64_t Timer::GetNowTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    uint64_t timestamp_ms = duration.count();
    return timestamp_ms;
}

void Timer::run() {
    uint64_t sleep = 50;
    while(!_close) {
        uint64_t now = GetNowTime();
        switch (_Type) {
            case MIN_HEAP: {
                updateMinHeapTimer(sleep, now);
            }
            break;
            case RBTREE: {
                updateRbtreeTimer(sleep, now);
            }
            break;
            case ZKIPLIST: {
                updateZskiplistTimer(sleep, now);
            }
            break;
            case TIMEWHEEL: {
                updateTimeWheelTimer(sleep, now);
            }
            break;
        }

        uint64_t now_new = GetNowTime();
        uint64_t diff = now_new - now;
        if (diff < sleep) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep - diff));
        } 
    }
}

void Timer::Start() {
    std::thread([this]() {
        this->run();
    }).detach();
}

void Timer::stop() {
    _close = true;
}

void Timer::operator()() {
    run();
}

void Timer::AddMinHeapTimer(uint64_t time, CallBack cb) {
    TimeNode* node = new TimeNode();
    node->env.time = GetNowTime() + time;
    node->cb = cb;
    min_heap_push(_heap, &node->env);
}

void Timer::AddRbtreeTimer(uint64_t time, CallBack cb) {
    TimeNodeRb* node = new TimeNodeRb();
    node->env.key = GetNowTime() + time;
    node->cb = cb;
    rbtree_insert(_rbtree, &node->env);
}

void Timer::AddZskiplistTimer(uint64_t time, CallBack cb) {
    TimeNodeZp* node = new TimeNodeZp();
    node->env.score = GetNowTime() + time;
    node->env.level = (zskiplistLevel*)zslCreateLevel(zslRandomLevel());
    node->cb = cb;
    zslInsert(_zskiplist, &node->env);
}

void Timer::AddTimeWheelTimer(uint64_t time, CallBack cb) {
    TimeNodeTw* node = new TimeNodeTw();
    node->cb = cb;
    timer_add(_timewheel, &node->env, static_cast<uint32_t>(time));
}

void Timer::updateMinHeapTimer(uint64_t& sleep, uint64_t& now) {
    timer_entry *entry = nullptr;
    if (!min_heap_top(_heap, &entry)) {
        if (entry->time <= now) {
            if (!min_heap_pop(_heap, &entry)) {
                TimeNode *node = container_of(entry, TimeNode, env);
                if (node) {
                    node->cb();
                }

                delete node;
            }
        } else {
            sleep = entry->time - now;
        }
    }
}

void Timer::updateRbtreeTimer(uint64_t& sleep, uint64_t& now) {
    rbtree_node* node = rbtree_min(_rbtree);
    if(!node) {
        return;
    }

    if(node->key <= now) {
        rbtree_delete(_rbtree, node);
        TimeNodeRb* n = container_of(node, TimeNodeRb, env);
        n->cb();

        delete n;
    } else {
        sleep = node->key - now; 
    }
}

void Timer::updateZskiplistTimer(uint64_t& sleep, uint64_t& now) {
    zskiplistNode* node = zslMin(_zskiplist);
    if(!node) {
        return;
    }

    if(node->score <= now) {
        zslDeleteHead(_zskiplist);
        TimeNodeZp* n = container_of(node, TimeNodeZp, env);
        if(n) {
            n->cb();
            zslDeleteNode(&n->env);
            delete n;
        }
    }else {
        sleep = node->score - now; 
    }
}

void Timer::updateTimeWheelTimer(uint64_t& sleep, uint64_t& now) {
    auto timerExecute = [&]() {
        int idx = _timewheel->time & TIME_NEAR_MASK;
	
	    while (_timewheel->near[idx].head.next) {
		    struct timer_node *current = link_clear(&_timewheel->near[idx]);
            if (!current) {
                break;
            }

		    SPIN_UNLOCK(_timewheel);
            do {
                struct timer_node * temp = current;
		        current = current->next;
                TimeNodeTw* n = container_of(temp, TimeNodeTw, env);
                if (n) {
                    n->cb();
                    delete n;
                }
            } while(current);
		    SPIN_LOCK(_timewheel);
	    }
    };
    SPIN_LOCK(_timewheel);
    timerExecute();

    timer_shift(_timewheel);

    timerExecute();
    SPIN_UNLOCK(_timewheel);
}