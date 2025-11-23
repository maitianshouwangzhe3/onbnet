
#include <sys/socket.h>
#include <iostream>
#include "socket_worker.h"
#include "net_worker_manager.h"
socket_worker::socket_worker(int max_connect, int port): is_stop_(false) {
    new net_worker_manager(max_connect);
}

socket_worker::~socket_worker() {

}

void socket_worker::start() {
    while (!is_stop_) {
        std::vector<onbnet::net::event> events;
        int ret = net_worker_manager_inst->poll(events, 50);
        if (ret <= 0) {
            continue;
        }

        for (auto &event: events) {
            auto connect = net_worker_manager_inst->get_connect(event.fd);
            if (connect) {
                connect->dispatch(event);
            }
        }
    }
}

void socket_worker::stop() {
    is_stop_ = true;
}