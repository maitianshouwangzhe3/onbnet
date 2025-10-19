
#include "Service.h"
#include "MessageWorker.h"
MessageWorker::MessageWorker(ProducerConsumerQueue<Service*>* q) {
    gqueue = q;
    mIsStop = false;
}

MessageWorker::~MessageWorker() {

}

void MessageWorker::start() {
    while(!mIsStop) {
        Service* s = gqueue->WaitAndPop();
        if (s) {
            s->Start();
        }
    }
}

void MessageWorker::stop() {
    mIsStop = true;
    gqueue->Cancel();
}