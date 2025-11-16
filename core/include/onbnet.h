#pragma once

#include <memory>
#include <vector>
#include <thread>

#include "base_worker.h"
#include "no_copy_able.h"
#include "config_file_reader.h"
#include "producer_consumer_queue.h"

class service;

#define onbnet_server_context onbnet_server::inst

class onbnet_server: public no_copy_able {
public:
    static onbnet_server* inst;

    onbnet_server(std::shared_ptr<onbnet::util::config_file_reader> config);
    ~onbnet_server();

    bool init();

    int start();

    void stop();


private:
    std::shared_ptr<onbnet::util::config_file_reader> config_;

    std::vector<onbnet::base_worker*> workers_;
    std::vector<std::unique_ptr<std::thread>> workerThreads_;
    producer_consumer_queue<service*> gqueue_;
};