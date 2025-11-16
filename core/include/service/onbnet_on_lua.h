#pragma once

#include "export.h"
#include "message.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

struct lua_State;
struct callback_context {
    lua_State* L;
};

class onbnet_on_lua final {
public:
    onbnet_on_lua();
    ~onbnet_on_lua();

    int operator()(std::shared_ptr<message>& msg);

    int init(void* ctx, std::string fileName);
    void callback();

    lua_State* lua_state_;
    callback_context* cb_context_;

    std::string lua_path_;
    std::string cpath_;
    std::string service_path_;
};

ONBNET_API onbnet_on_lua* new_onbnet_on_lua();