
#include "timer.h"
#include "util.h"
#include "onbnet.h"
#include "logger.h"
#include "socket_worker.h"
#include "message_worker.h"
#include "service_manager.h"
#include "config_file_reader.h"


#include <memory>
#include <stdexcept>
#include <thread>

onbnet_server* onbnet_server::inst = nullptr;
onbnet_server::onbnet_server(std::shared_ptr<onbnet::util::config_file_reader> config) {
    if(!inst) {
        inst = this;
    } else {
        throw std::runtime_error("onbnet instance already exists");
    }

    config_ = config;
}

onbnet_server::~onbnet_server() {
    if (LOGGER) {
        delete LOGGER;
    }
}

bool onbnet_server::init() {
    if (!config_) {
        return false;
    }
    int threadCount = 2;
    char* tmp = config_->get_config_name("threads");
    if (tmp) {
        threadCount = onbnet::util::string2int(tmp);
    }

    int maxConnect = 1024;
    tmp = config_->get_config_name("max_connect");
    if (tmp) {
        maxConnect = onbnet::util::string2int(tmp);
    }
    
    workers_.emplace_back(new socket_worker(maxConnect, 0));
    workerThreads_.emplace_back(new std::thread([&](){
        workers_[0]->start();
    }));

    for (int i = workers_.size(); i < threadCount; ++i) {
        auto worker = new message_worker(&gqueue_);
        workers_.emplace_back(worker);
        workerThreads_.emplace_back(new std::thread([worker]() {
            worker->start();
        }));
    }

    new onbnet::tw::timer(onbnet::tw::time_type::TIMEWHEEL);
    timer_inst->start();
    return true;
}

int onbnet_server::start() {
    auto sm = new service_manager();
    sm->init(config_.get());
    sm->set_queue(&gqueue_);
    char* tmp = config_->get_config_name("bootstrap");
    if (tmp) {
        std::string boot = config_->get_config_name("bootstrap");
        sm->new_service(boot.c_str());
    } else {
        throw std::runtime_error("no bootstrap");
    }

    for (int i = 0; i < workerThreads_.size(); ++i) {
        workerThreads_[i]->join();
    }

    return 0;
}

void onbnet_server::stop() {
    log_info("onbnet_server workers stop");
    for (int i = 0; i < workers_.size(); ++i) {
        workers_[i]->stop();
    }

    log_info("onbnet_server Timer stop");
    timer_inst->stop();
}