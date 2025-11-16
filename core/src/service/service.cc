
#include "service.h"
#include "message.h"
#include "logger.h"
#include "onbnet_on_lua.h"
#include "service_manager.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>

service::service(std::string file_name): file_name_(file_name), session_(10) {
    Invoke_.store(false);
}

service::~service() {
    gqueue.Cancel();
}

void service::init() {
    lua_state_ = new onbnet_on_lua();
    intptr_t p = (intptr_t)this;
    memcpy(lua_getextraspace(lua_state_->lua_state_), &p, LUA_EXTRASPACE);
    lua_state_->lua_path_ = lua_path_;
    lua_state_->cpath_ = cpath_;
    lua_state_->service_path_ = service_path_;
    lua_state_->init(static_cast<void*>(this), file_name_);
}

bool service::start() {
    int n = gqueue.Size();
    for (int i = 0; i < n; i++) {
        std::shared_ptr<message> msg = gqueue.PopPtr();
        if (msg) {
            try {
                (*lua_state_)(msg);
            } catch (...) {
                log_error("message callback error");
            }
        } else {
            log_error("service::Start gqueue.PopPtr() is null");
        }
    }

    if (!gqueue.Empty()) {
        service_manager_inst->push_global_msg_queue(this);
        return true;
    }

    Invoke_.store(false);
    return false;
}

void service::push_message(std::shared_ptr<message> msg) {
    gqueue.PushPtr(msg);
}

int service::set_callback() {
    lua_state_->callback();
    return 0;
}

service* get_service(lua_State* L) {
    static_assert((LUA_EXTRASPACE == sizeof(service*)) && (LUA_EXTRASPACE == sizeof(intptr_t)));
    intptr_t v = 0;
    memcpy(&v, lua_getextraspace(L), LUA_EXTRASPACE);
    return reinterpret_cast<service*>(v);
}

uint64_t service::get_session() {
    return session_++;
}