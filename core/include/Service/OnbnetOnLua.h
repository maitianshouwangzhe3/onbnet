#pragma once

#include "export.h"
#include "Message.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

struct lua_State;
struct CallbackContext {
    lua_State* L;
};

class OnbnetOnLua final {
public:
    OnbnetOnLua();
    ~OnbnetOnLua();

    int operator()(std::shared_ptr<Message>& msg);

    int Init(void* ctx, std::string fileName);
    void CallBack();

    lua_State* LuaState;
    CallbackContext* cbContext;

    std::string luaPath;
    std::string cPath;
    std::string servicePath;
};

ONBNET_API OnbnetOnLua* newOnbnetOnLua();