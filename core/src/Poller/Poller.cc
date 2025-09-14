
#include "Poller.h"

#include <cstdint>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
Poller::Poller() {
    mEpfd = epoll_create1(EPOLL_CLOEXEC);
}
Poller::~Poller() {
    close(mEpfd);
}

int Poller::Poll(std::vector<Event>& events, int timeout) {
    epoll_event ev[128] = {};
    int nready = epoll_wait(mEpfd, ev, 128, timeout);
    if (nready > 0) {
        for (int i = 0; i < nready; i++) {
            Event event;
            if (ev[i].events & EPOLLRDHUP || ev[i].events & EPOLLHUP) {
                event.event |= static_cast<uint32_t>(EventType::EVENT_RDHUP);
            } else {
                if (ev[i].events & EPOLLIN) {
                    event.event |= static_cast<uint32_t>(EventType::EVENT_READ);
                }

                if (ev[i].events & EPOLLOUT) {
                    event.event |= static_cast<uint32_t>(EventType::EVENT_WRITE);
                }
            }
            
            event.fd = ev[i].data.fd;
            event.ptr = ev[i].data.ptr;
            events.emplace_back(std::move(event));
        }
    }

    return nready;
}

int Poller::AddEventRead(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLIN | EPOLLET;
    } else {
        ev.events = EPOLLIN;
    }
    ev.data.fd = fd;
    return epoll_ctl(mEpfd, EPOLL_CTL_ADD, fd, &ev);
}

int Poller::AddEventWrite(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLOUT | EPOLLET;
    } else {
        ev.events = EPOLLOUT;
    }
    ev.data.fd = fd;
    return epoll_ctl(mEpfd, EPOLL_CTL_ADD, fd, &ev);
}

int Poller::ModEventRead(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLIN | EPOLLET;
    } else {
        ev.events = EPOLLIN;
    }
    ev.data.fd = fd;
    return epoll_ctl(mEpfd, EPOLL_CTL_MOD, fd, &ev);

}

int Poller::ModEventWrite(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLOUT | EPOLLET;
    } else {
        ev.events = EPOLLOUT;
    }
    ev.data.fd = fd;
    return epoll_ctl(mEpfd, EPOLL_CTL_MOD, fd, &ev);
}

int Poller::ModEventReadWrite(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    } else {
        ev.events = EPOLLIN | EPOLLOUT;
    }
    ev.data.fd = fd;
    return epoll_ctl(mEpfd, EPOLL_CTL_MOD, fd, &ev);
}

int Poller::DelEvent(int fd) {
    return epoll_ctl(mEpfd, EPOLL_CTL_DEL, fd, nullptr);
}