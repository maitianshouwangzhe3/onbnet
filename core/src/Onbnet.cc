
#include "util.h"
#include "Onbnet.h"
#include "SocketWorker.h"
#include "MessageWorker.h"
#include "ServiceManager.h"
#include "ConfigFileReader.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

onbnetServer* onbnetServer::inst = nullptr;
onbnetServer::onbnetServer(std::shared_ptr<CConfigFileReader> config) {
    if(!inst) {
        inst = this;
    } else {
        throw std::runtime_error("onbnet instance already exists");
    }

    mConfig = config;
}

onbnetServer::~onbnetServer() {

}

bool onbnetServer::Init() {
    if (!mConfig) {
        return false;
    }
    int threadCount = 2;
    char* tmp = mConfig->GetConfigName("threads");
    if (tmp) {
        threadCount = String2Int(tmp);
    }

    int maxConnect = 1024;
    tmp = mConfig->GetConfigName("max_connect");
    if (tmp) {
        maxConnect = String2Int(tmp);
    }
    
    workers.emplace_back(new SocketWorker(maxConnect, 0));
    workerThreads.emplace_back(new std::thread([&](){
        workers[0]->start();
    }));

    for (int i = workers.size(); i < threadCount; ++i) {
        auto worker = new MessageWorker(&gqueue);
        workers.emplace_back(worker);
        workerThreads.emplace_back(new std::thread([worker]() {
            worker->start();
        }));
    }

    return true;
}

int onbnetServer::Start() {
    auto sm = new ServiceManager();
    sm->Init(mConfig.get());
    sm->SetQueue(&gqueue);
    char* tmp = mConfig->GetConfigName("bootstrap");
    if (tmp) {
        std::string boot = mConfig->GetConfigName("bootstrap");
        sm->newService(boot.c_str());
    } else {
        std::cout << "no bootstrap" << std::endl;
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    for (int i = 0; i < workerThreads.size(); ++i) {
        workerThreads[i]->join();
    }
    return 0;
}

void onbnetServer::Stop() {
    std::cout << "onbnetServer stop" << std::endl;
    for (int i = 0; i < workers.size(); ++i) {
        workers[i]->stop();
    }
}