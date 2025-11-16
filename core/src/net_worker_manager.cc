
#include "net_worker_manager.h"
#include "poller.h"
#include <stdexcept>

net_worker_manager* net_worker_manager::inst = nullptr;

net_worker_manager::net_worker_manager(int max_connect): max_connect_(max_connect) {
    if (!inst) {
        inst = this;
    } else {
        throw std::runtime_error("net_worker_manager::inst is not null");
    }

    connects_.reserve(max_connect_ + 1);
    poller_ = std::make_shared<onbnet::net::poller>();
}

net_worker_manager::~net_worker_manager() {

}

int net_worker_manager::add_connect(int fd) {
    return 0;
}

int net_worker_manager::add_connect(std::shared_ptr<onbnet::net::connect> connect, bool isEt) {
    int fd = connect->get_socket()->get_fd();
    if (fd > connects_.capacity()) {
        connects_.reserve(fd + 1);
    }

    if (!connects_[fd]) {
        connects_[fd] = connect;
    }
    // poller_->AddEventRead(fd, nullptr, isEt);
    return 0;
}

int net_worker_manager::event_read(int fd, bool isEt) {
    return poller_->add_event_read(fd, isEt);
}

int net_worker_manager::event_write(int fd, bool isEt) {
    return poller_->add_event_write(fd, isEt);
}

int net_worker_manager::event_mod_read_write(int fd, bool isEt) {
    return poller_->mod_event_read_write(fd, isEt);
}

int net_worker_manager::event_mod_read(int fd, bool isEt) {
    return poller_->mod_event_read(fd, isEt);
}

int net_worker_manager::delete_connect(int fd) {
    poller_->del_event(fd);
    connects_[fd].reset();
    if (service_map_.find(fd) != service_map_.end()) {
        service_map_.erase(fd);
    }
    return 0;
}

std::shared_ptr<onbnet::net::connect> net_worker_manager::get_connect(int fd) {
    if(fd >= max_connect_ || fd < 0) {
        return nullptr;
    }

    return connects_[fd];
}

int net_worker_manager::poll(std::vector<onbnet::net::event>& events, int timeout) {
    return poller_->poll(events, timeout);
}

service* net_worker_manager::get_service(int id) {
    auto service = service_map_.find(id);
    if (service != service_map_.end()) {
        return service->second;
    }

    return nullptr;
}

void net_worker_manager::set_service(int id, service* service) {
    service_map_.insert(std::make_pair(id, service));
}