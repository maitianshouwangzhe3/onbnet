#pragma once

#include <vector>

enum class EventType {
    EVENT_UNKNOWN = 1,
    EVENT_READ = 1 << 1,
    EVENT_WRITE = 1 << 2,
    EVENT_RDHUP = 1 << 3
};

struct Event {
    EventType event;
    int fd;
    void* ptr;
};

class Poller {
public:
    Poller();
    ~Poller();

    int Poll(std::vector<Event>& events, int timeout = -1);

    int AddEventRead(int fd, void* data = nullptr, bool isEt = true);

    int AddEventWrite(int fd, void* data = nullptr, bool isEt = true);

    int ModEventRead(int fd, void* data = nullptr, bool isEt = true);

    int ModEventWrite(int fd, void* data = nullptr, bool isEt = true);

    int DelEvent(int fd);
private:
    int mEpfd;
};