#pragma once

#include <memory>
#include <vector>
#include <thread>

#include "baseWorker.h"
#include "NoCopyAble.h"
#include "ProducerConsumerQueue.h"

class Service;
class CConfigFileReader;

#define onbnetServerContext onbnetServer::inst

class onbnetServer: public NonCopyAble {
public:
    static onbnetServer* inst;

    onbnetServer(std::shared_ptr<CConfigFileReader> config);
    ~onbnetServer();

    bool Init();

    int Start();

    void Stop();

    

private:
    std::shared_ptr<CConfigFileReader> mConfig;
    std::vector<onbnet::baseWorker*> workers;
    std::vector<std::unique_ptr<std::thread>> workerThreads;

    ProducerConsumerQueue<Service*> gqueue;
};