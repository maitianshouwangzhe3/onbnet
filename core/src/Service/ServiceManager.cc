
#include "Service.h"
#include "logger.h"
#include "luna.h"
#include <atomic>
#include <stdexcept>
#include "ServiceManager.h"
#include "ConfigFileReader.h"

LUA_EXPORT_CLASS_BEGIN(ServiceManager)
LUA_EXPORT_METHOD(newService)
LUA_EXPORT_CLASS_END()


ServiceManager* ServiceManager::inst = nullptr;

ServiceManager::ServiceManager() {
    if (!inst) {
        inst = this;
    } else {
        throw std::runtime_error("ServiceManager instance already exists");
    }
}

ServiceManager::~ServiceManager() {

}

void ServiceManager::Init(CConfigFileReader* config) {
    maxServiceId = 0;
    mServiceVector.reserve(24);

    char* tmp = config->GetConfigName("lua_path");
    if (tmp) {
        luaPath = tmp;
    } else {
        throw std::runtime_error("lua_path not found");
    }
    
    tmp = config->GetConfigName("lua_cpath");
    if (tmp) {
        cPath = tmp;
    } else {
        throw std::runtime_error("lua_cpath not found");
    }

    tmp = config->GetConfigName("lua_service");
    if (tmp) {
        servicePath = tmp;
    } else {
        throw std::runtime_error("lua_service not found");
    }
}

void ServiceManager::__gc() {
    
}

void ServiceManager::Update() {

}

void ServiceManager::CloseService(uint32_t serviceId) {
    mServiceVector[serviceId] = nullptr;
}

bool ServiceManager::Send(std::shared_ptr<Message> msg) {
    auto S = mServiceVector[msg->source];
    if (S) {
        S->PushMessage(msg);
        queue->Push(S);
        return true;
    }
    return false;
}

bool ServiceManager::Send(uint32_t serviceId, std::shared_ptr<Message> msg) {
    auto S = mServiceVector[serviceId];
    if (S) {
        S->PushMessage(msg);
        bool oldv = false;
        bool newv = true;
        if (S->Invoke.compare_exchange_weak(oldv, newv, std::memory_order_release, std::memory_order_release)) {
            queue->Push(S);
        }
        
        return true;
    }
    return false;
}

int ServiceManager::newService(const char* serviceName) {
    Service* service = new Service(serviceName);
    service->ServiceId = maxServiceId++;
    service->luaPath = luaPath;
    service->cPath = cPath;
    service->servicePath = servicePath;
    service->Init();
    mServiceVector[service->ServiceId] = service;
    log_info("new service {} [{}]", serviceName, service->ServiceId);
    return service->ServiceId;
}

void ServiceManager::SetQueue(ProducerConsumerQueue<Service*>* q) {
    queue = q;
}

void ServiceManager::PushglobalMsgQueue(Service* s) {
    queue->Push(s);
}