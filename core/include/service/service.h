#pragma once

#include "onbnet_on_lua.h"
#include "export.h"

#include "luna.h"
#include "message.h"

#include <cstdint>
#include <string>
#include <memory>
#include <sys/types.h>


#include "producer_consumer_queue.h"

class onbnet_on_lua;

class service final {
public:
    service(std::string file_name);
    ~service();

    void init();

    bool start();

    void test();

    int set_callback();

    void push_message(std::shared_ptr<message> msg);

    uint64_t get_session();

    // DECLARE_LUA_CLASS(Service);

    onbnet_on_lua* lua_state_;
    uint32_t service_id_;
    std::string file_name_;
    std::string lua_path_;
    std::string cpath_;
    std::string service_path_;
    uint64_t session_;

    std::atomic<bool> Invoke_;

    producer_consumer_queue<std::shared_ptr<message>> gqueue;
};

service* get_service(lua_State* L);