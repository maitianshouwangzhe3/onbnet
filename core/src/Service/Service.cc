
#include "Service.h"
#include "Message.h"
#include "OnbnetOnLua.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

// LUA_EXPORT_CLASS_BEGIN(Service)
// // LUA_EXPORT_METHOD(Init)
// // LUA_EXPORT_METHOD(Start)
// LUA_EXPORT_METHOD(test)
// LUA_EXPORT_PROPERTY(ServiceId)
// LUA_EXPORT_CLASS_END()

Service::Service(std::string fileName): FileName(fileName) {

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
    //调用Lua函数
    // lua_getglobal(LuaState->LuaState, "OnInit"); 
    // lua_pushinteger(LuaState->LuaState, 1); 
    // int isok = lua_pcall(LuaState->LuaState, 1, 0, 0);
    // if(isok != 0){ //成功返回值为0，否则代表失败.
    //     std::cout << "call lua OnInit fail " << lua_tostring(LuaState->LuaState, -1) << std::endl;
    // }

    while (!gqueue.Empty()) {
        // Message m;
        // gqueue.Pop(m);

        std::shared_ptr<Message> msg = gqueue.PopPtr();
        std::cout << "Service::Start" << std::endl;
        (*LuaState)(msg);
        // (*LuaState)(m.type, m.session, m.source, m.data, m.sz);
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