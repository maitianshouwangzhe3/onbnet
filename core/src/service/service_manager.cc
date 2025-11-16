
#include "service.h"
#include "logger.h"
#include "luna.h"
#include <atomic>
#include <stdexcept>
#include "service_manager.h"
#include "config_file_reader.h"

LUA_EXPORT_CLASS_BEGIN(service_manager)
LUA_EXPORT_METHOD(new_service)
LUA_EXPORT_CLASS_END()


service_manager* service_manager::inst = nullptr;

service_manager::service_manager() {
    if (!inst) {
        inst = this;
    } else {
        throw std::runtime_error("service_manager instance already exists");
    }
}

service_manager::~service_manager() {

}

void service_manager::init(onbnet::util::config_file_reader* config) {
    max_service_id = 0;
    service_vector_.reserve(24);

    char* tmp = config->get_config_name("lua_path");
    if (tmp) {
        lua_path_ = tmp;
    } else {
        throw std::runtime_error("lua_path not found");
    }
    
    tmp = config->get_config_name("lua_cpath");
    if (tmp) {
        cpath_ = tmp;
    } else {
        throw std::runtime_error("lua_cpath not found");
    }

    tmp = config->get_config_name("lua_service");
    if (tmp) {
        service_path_ = tmp;
    } else {
        throw std::runtime_error("lua_service not found");
    }
}

void service_manager::__gc() {
    
}

void service_manager::update() {

}

void service_manager::close_service(uint32_t service_id) {
    service_vector_[service_id] = nullptr;
}

bool service_manager::send(std::shared_ptr<message> msg) {
    auto S = service_vector_[msg->source];
    if (S) {
        S->push_message(msg);
        queue->Push(S);
        return true;
    }
    return false;
}

bool service_manager::send(uint32_t serviceId, std::shared_ptr<message> msg) {
    auto S = service_vector_[serviceId];
    if (S) {
        S->push_message(msg);
        bool oldv = false;
        bool newv = true;
        if (S->Invoke_.compare_exchange_weak(oldv, newv, std::memory_order_release, std::memory_order_release)) {
            queue->Push(S);
        }
        
        return true;
    }
    return false;
}

int service_manager::new_service(const char* service_name) {
    service* s = new service(service_name);
    s->service_id_ = max_service_id++;
    s->lua_path_ = lua_path_;
    s->cpath_ = cpath_;
    s->service_path_ = service_path_;
    s->init();
    service_vector_[s->service_id_] = s;
    log_info("new service {} [{}]", service_name, s->service_id_);
    return s->service_id_;
}

void service_manager::set_queue(producer_consumer_queue<service*>* q) {
    queue = q;
}

void service_manager::push_global_msg_queue(service* s) {
    queue->Push(s);
}