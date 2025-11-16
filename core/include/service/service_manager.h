#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>


#include "luna.h"
#include "export.h"
#include "message.h"
#include "config_file_reader.h"
#include "producer_consumer_queue.h"

class service;

class ONBNET_API service_manager final {
public:
    service_manager();
    ~service_manager();
    void __gc();
    void update();
    void close_service(uint32_t service_id);
    bool send(std::shared_ptr<message> msg);
    bool send(uint32_t service_id, std::shared_ptr<message> msg);
    int new_service(const char* service_name);
    void set_queue(producer_consumer_queue<service*>* q);

    void init(onbnet::util::config_file_reader* config);

    void push_global_msg_queue(service* s);

    DECLARE_LUA_CLASS(service_manager)
    static service_manager* inst;
private:
    uint32_t max_service_id;
    std::vector<service*> service_vector_;

    std::string lua_path_;
    std::string cpath_;
    std::string service_path_;

    producer_consumer_queue<service*>* queue;
};

#define service_manager_inst service_manager::inst