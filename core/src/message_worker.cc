
#include "service.h"
#include "message_worker.h"
message_worker::message_worker(producer_consumer_queue<service*>* q) {
    gqueue = q;
    is_stop_ = false;
}

message_worker::~message_worker() {

}

void message_worker::start() {
    while(!is_stop_) {
        service* s = gqueue->WaitAndPop();
        if (s) {
            s->start();
        }
    }
}

void message_worker::stop() {
    is_stop_ = true;
    gqueue->Cancel();
}