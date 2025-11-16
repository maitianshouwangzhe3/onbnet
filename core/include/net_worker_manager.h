#pragma once

#include <memory>
#include <unordered_map>
#include <vector>


#include "poller.h"
#include "connect.h"
#include "service.h"

#define net_worker_manager_inst net_worker_manager::inst

class net_worker_manager { 
public:
    static net_worker_manager* inst;
    net_worker_manager() = default;
    net_worker_manager(int max_connect = 1024);
    net_worker_manager(net_worker_manager&) = delete;
    ~net_worker_manager();

    int poll(std::vector<onbnet::net::event>& events, int timeout = -1);

    int add_connect(int fd);
    int add_connect(std::shared_ptr<onbnet::net::connect> connect, bool isEt = true);
    int delete_connect(int fd);

    std::shared_ptr<onbnet::net::connect> get_connect(int fd);

    service* get_service(int id);

    void set_service(int id, service* service);

    int event_read(int fd, bool is_et = true);

    int event_write(int fd, bool is_et = true);

    int event_mod_read_write(int fd, bool is_et = true);

    int event_mod_read(int fd, bool is_et = true);

private:
    int max_connect_;
    std::vector<std::shared_ptr<onbnet::net::connect>> connects_;
    std::shared_ptr<onbnet::net::poller> poller_;

    std::unordered_map<int, service*> service_map_;
};