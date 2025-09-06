#pragma once

#include <memory>
#include <unordered_map>
#include <vector>


#include "Poller.h"
#include "Connect.h"
#include "Service.h"

#define NetWorkerManagerInst NetWorkerManager::inst

class NetWorkerManager { 
public:
    static NetWorkerManager* inst;
    NetWorkerManager() = default;
    NetWorkerManager(int maxConnect = 1024);
    NetWorkerManager(NetWorkerManager&) = delete;
    ~NetWorkerManager();

    int Poll(std::vector<Event>& events, int timeout = -1);

    int AddConnect(int fd);
    int AddConnect(std::shared_ptr<Connect> connect, bool isEt = true);
    int DeleteConnect(int fd);

    std::shared_ptr<Connect> GetConnect(int fd);

    Service* GetService(int id);

    void SetService(int id, Service* service);

    int eventRead(int fd, bool isEt = true);

private:
    int mMaxConnect;
    std::vector<std::shared_ptr<Connect>> mConnects;
    std::shared_ptr<Poller> mPoller;

    std::unordered_map<int, Service*> mService;
};