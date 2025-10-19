
#include "Service.h"
#include "Message.h"
#include "logger.h"
#include "OnbnetOnLua.h"
#include "ServiceManager.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>

Service::Service(std::string fileName): FileName(fileName), session(10) {
    Invoke.store(false);
}

Service::~Service() {
    gqueue.Cancel();
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

bool Service::Start() {
    int n = gqueue.Size();
    for (int i = 0; i < n; i++) {
        std::shared_ptr<Message> msg = gqueue.PopPtr();
        if (msg) {
            try {
                (*LuaState)(msg);
            } catch (...) {
                log_error("message callback error");
            }
        } else {
            log_error("Service::Start gqueue.PopPtr() is null");
        }
    }

    if (!gqueue.Empty()) {
        ServiceManagerInst->PushglobalMsgQueue(this);
        return true;
    }

    Invoke.store(false);
    return false;
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