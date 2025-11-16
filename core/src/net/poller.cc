
#include "poller.h"

#include <cstdint>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
onbnet::net::poller::poller() {
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
}
onbnet::net::poller::~poller() {
    close(epfd_);
}

int onbnet::net::poller::poll(std::vector<event>& events, int timeout) {
    epoll_event evs[128] = {};
    int nready = epoll_wait(epfd_, evs, 128, timeout);
    if (nready > 0) {
        for (int i = 0; i < nready; i++) {
            event ev;
            if (evs[i].events & EPOLLRDHUP || evs[i].events & EPOLLHUP) {
                ev.event_ |= static_cast<uint32_t>(event_type::EVENT_RDHUP);
            } else {
                if (evs[i].events & EPOLLIN) {
                    ev.event_ |= static_cast<uint32_t>(event_type::EVENT_READ);
                }

                if (evs[i].events & EPOLLOUT) {
                    ev.event_ |= static_cast<uint32_t>(event_type::EVENT_WRITE);
                }
            }
            
            ev.fd = evs[i].data.fd;
            ev.ptr = evs[i].data.ptr;
            events.emplace_back(std::move(ev));
        }
    }

    return nready;
}

int onbnet::net::poller::add_event_read(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLIN | EPOLLET;
    } else {
        ev.events = EPOLLIN;
    }
    ev.data.fd = fd;
    return epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

int onbnet::net::poller::add_event_write(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLOUT | EPOLLET;
    } else {
        ev.events = EPOLLOUT;
    }
    ev.data.fd = fd;
    return epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

int onbnet::net::poller::mod_event_read(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLIN | EPOLLET;
    } else {
        ev.events = EPOLLIN;
    }
    ev.data.fd = fd;
    return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);

}

int onbnet::net::poller::mod_event_write(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLOUT | EPOLLET;
    } else {
        ev.events = EPOLLOUT;
    }
    ev.data.fd = fd;
    return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

int onbnet::net::poller::mod_event_read_write(int fd, bool isEt) {
    epoll_event ev;
    if (isEt) {
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    } else {
        ev.events = EPOLLIN | EPOLLOUT;
    }
    ev.data.fd = fd;
    return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

int onbnet::net::poller::del_event(int fd) {
    return epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
}