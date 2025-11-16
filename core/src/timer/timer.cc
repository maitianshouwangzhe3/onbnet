
#include "timer.h"

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <thread>

onbnet::tw::timer* onbnet::tw::timer::inst = nullptr;

onbnet::tw::timer::timer(uint32_t size) {
    if (!inst) {
        timer::inst = this;
    } else {
        throw std::runtime_error("timer::inst is not null");
    }

    min_heap_init(&_heap, size);
    _close = false;
}

onbnet::tw::timer::timer(time_type Type, uint32_t size) {
    if (!inst) {
        timer::inst = this;
    } else {
        throw std::runtime_error("timer::inst is not null");
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
        _zskiplist = zsl_create();
        break;
    case TIMEWHEEL:
        _timewheel = onbnet_timer_create();
        break;
    }
    _close = false;
    _Type = Type;
}

onbnet::tw::timer::~timer() {
    switch (_Type) {
        case MIN_HEAP:
            min_heap_free(_heap);
        break;
        case RBTREE:
            delete _rbtree->sentinel;
            delete _rbtree;      
        break;
        case ZKIPLIST:
            zsl_free(_zskiplist);
        break;
        case TIMEWHEEL:
        break;
    }
}

int onbnet::tw::timer::add_timer(uint64_t time, callback cb) {
    switch (_Type) {
    case MIN_HEAP:
        add_min_heap_timer(time, cb);
        break;
    case RBTREE:
        add_rbtree_timer(time, cb);
        break;
    case ZKIPLIST:
        add_zskiplist_timer(time, cb);
        break;
    case TIMEWHEEL:
        add_time_wheel_timer(time, cb);
        break;
    }
    return 0;
}

uint64_t onbnet::tw::timer::get_now_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    uint64_t timestamp_ms = duration.count();
    return timestamp_ms;
}

void onbnet::tw::timer::run() {
    uint64_t sleep = 50;
    while(!_close) {
        uint64_t now = get_now_time();
        switch (_Type) {
            case MIN_HEAP: {
                update_min_heap_timer(sleep, now);
            }
            break;
            case RBTREE: {
                update_rbtree_timer(sleep, now);
            }
            break;
            case ZKIPLIST: {
                update_zskiplist_timer(sleep, now);
            }
            break;
            case TIMEWHEEL: {
                update_time_wheel_timer(sleep, now);
            }
            break;
        }

        uint64_t now_new = get_now_time();
        uint64_t diff = now_new - now;
        if (diff < sleep) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep - diff));
        } 
    }
}

void onbnet::tw::timer::start() {
    std::thread([this]() {
        this->run();
    }).detach();
}

void onbnet::tw::timer::stop() {
    _close = true;
}

void onbnet::tw::timer::operator()() {
    run();
}

void onbnet::tw::timer::add_min_heap_timer(uint64_t time, callback cb) {
    time_node* node = new time_node();
    node->env.time = get_now_time() + time;
    node->cb = cb;
    min_heap_push(_heap, &node->env);
}

void onbnet::tw::timer::add_rbtree_timer(uint64_t time, callback cb) {
    time_node_rb* node = new time_node_rb();
    node->env.key = get_now_time() + time;
    node->cb = cb;
    rbtree_insert(_rbtree, &node->env);
}

void onbnet::tw::timer::add_zskiplist_timer(uint64_t time, callback cb) {
    time_node_zp* node = new time_node_zp();
    node->env.score = get_now_time() + time;
    node->env.level = (zskiplistLevel*)zsl_create_level(zsl_random_level());
    node->cb = cb;
    zsl_insert(_zskiplist, &node->env);
}

void onbnet::tw::timer::add_time_wheel_timer(uint64_t time, callback cb) {
    time_node_tw* node = new time_node_tw();
    node->cb = cb;
    timer_wheel_add(_timewheel, &node->env, static_cast<uint32_t>(time));
}

void onbnet::tw::timer::update_min_heap_timer(uint64_t& sleep, uint64_t& now) {
    timer_entry *entry = nullptr;
    if (!min_heap_top(_heap, &entry)) {
        if (entry->time <= now) {
            if (!min_heap_pop(_heap, &entry)) {
                time_node *node = container_of(entry, time_node, env);
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

void onbnet::tw::timer::update_rbtree_timer(uint64_t& sleep, uint64_t& now) {
    rbtree_node* node = rbtree_min(_rbtree);
    if(!node) {
        return;
    }

    if(node->key <= now) {
        rbtree_delete(_rbtree, node);
        time_node_rb* n = container_of(node, time_node_rb, env);
        n->cb();

        delete n;
    } else {
        sleep = node->key - now; 
    }
}

void onbnet::tw::timer::update_zskiplist_timer(uint64_t& sleep, uint64_t& now) {
    zskiplist_node* node = zsl_min(_zskiplist);
    if(!node) {
        return;
    }

    if(node->score <= now) {
        zsl_delete_head(_zskiplist);
        time_node_zp* n = container_of(node, time_node_zp, env);
        if(n) {
            n->cb();
            zsl_delete_node(&n->env);
            delete n;
        }
    }else {
        sleep = node->score - now; 
    }
}

void onbnet::tw::timer::update_time_wheel_timer(uint64_t& sleep, uint64_t& now) {
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
                time_node_tw* n = container_of(temp, time_node_tw, env);
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