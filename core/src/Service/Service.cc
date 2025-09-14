
#include "Service.h"
#include "Message.h"
#include "OnbnetOnLua.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

Service::Service(std::string fileName): FileName(fileName), session(10) {

}

Service::~Service() {
    std::cout << "Service::~Service" << std::endl;
}

void Service::Init() {
    LuaState = newOnbnetOnLua();
    intptr_t p = (intptr_t)this;
    memcpy(lua_getextraspace(LuaState->LuaState), &p, LUA_EXTRASPACE);
    LuaState->luaPath = luaPath;
    LuaState->cPath = cPath;
    LuaState->servicePath = servicePath;
    LuaState->Init(static_cast<void*>(this), FileName);
}

void Service::Start() {
    while (!gqueue.Empty()) {
        std::shared_ptr<Message> msg = gqueue.PopPtr();
        (*LuaState)(msg);
    }
}

void Service::PushMessage(std::shared_ptr<Message> msg) {
    gqueue.PushPtr(msg);
}

int Service::setCallBack() {
    LuaState->CallBack();
    return 0;
}

Service* GetService(lua_State* L) {
    static_assert((LUA_EXTRASPACE == sizeof(Service*)) && (LUA_EXTRASPACE == sizeof(intptr_t)));
    intptr_t v = 0;
    memcpy(&v, lua_getextraspace(L), LUA_EXTRASPACE);
    return reinterpret_cast<Service*>(v);
}

uint64_t Service::GetSession() {
    return session++;
}