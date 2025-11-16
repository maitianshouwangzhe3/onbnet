#pragma once

#include "base_worker.h"
#include "producer_consumer_queue.h"

class service;

class message_worker: public onbnet::base_worker {
public:
    message_worker(producer_consumer_queue<service*>* q);
    ~message_worker();

    virtual void start() override;
    virtual void stop() override;

    producer_consumer_queue<service*>* gqueue;
private:
    bool is_stop_;
};