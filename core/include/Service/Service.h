#pragma once

#include "OnbnetOnLua.h"
#include "export.h"

#include "luna.h"
#include "Message.h"

#include <cstdint>
#include <string>
#include <memory>


#include "ProducerConsumerQueue.h"

class OnbnetOnLua;

class Service final {
public:
    Service(std::string fileName);
    ~Service();

    void Init();

    void Start();

    void test();

    int setCallBack();

    void PushMessage(std::shared_ptr<Message> msg);

    // DECLARE_LUA_CLASS(Service);

    OnbnetOnLua* LuaState;
    uint32_t ServiceId;
    std::string FileName;
    std::string luaPath;
    std::string cPath;
    std::string servicePath;

    ProducerConsumerQueue<std::shared_ptr<Message>> gqueue;
};

Service* GetService(lua_State* L);