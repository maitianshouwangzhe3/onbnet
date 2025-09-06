#pragma once

#include "baseWorker.h"
#include "ProducerConsumerQueue.h"

class Service;

class MessageWorker: public onbnet::baseWorker {
public:
    MessageWorker(ProducerConsumerQueue<Service*>* q);
    ~MessageWorker();

    virtual void start() override;
    virtual void stop() override;

    ProducerConsumerQueue<Service*>* gqueue;
private:
    bool mIsStop;
};