#pragma once

#include <cstdint>
#include <vector>

namespace onbnet::net {

enum class event_type {
    EVENT_UNKNOWN = 1,
    EVENT_READ = 1 << 1,
    EVENT_WRITE = 1 << 2,
    EVENT_RDHUP = 1 << 3
};

struct event {
    event() {
        event_ = 0;
        fd = -1;
        ptr = nullptr;
    }
    uint32_t event_;
    int fd;
    void* ptr;
};

class poller {
public:
    poller();
    ~poller();

    int poll(std::vector<event>& events, int timeout = -1);

    int add_event_read(int fd, bool isEt = true);

    int add_event_write(int fd, bool isEt = true);

    int mod_event_read(int fd, bool isEt = true);

    int mod_event_write(int fd, bool isEt = true);

    int mod_event_read_write(int fd, bool isEt = true);

    int del_event(int fd);
private:
    int epfd_;
};

};