#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>


#include "luna.h"
#include "export.h"
#include "Message.h"
#include "ProducerConsumerQueue.h"

class Service;
class CConfigFileReader;

class ONBNET_API ServiceManager final {
public:
    ServiceManager();
    ~ServiceManager();
    void __gc();
    void Update();
    void CloseService(uint32_t serviceId);
    bool Send(std::shared_ptr<Message> msg);
    int newService(const char* serviceName);
    void SetQueue(ProducerConsumerQueue<Service*>* q);

    void Init(CConfigFileReader* config);

    DECLARE_LUA_CLASS(ServiceManager)
    static ServiceManager* inst;
private:
    uint32_t maxServiceId;
    std::vector<Service*> mServiceVector;

    std::string luaPath;
    std::string cPath;
    std::string servicePath;

    ProducerConsumerQueue<Service*>* queue;
};

#define ServiceManagerInst ServiceManager::inst