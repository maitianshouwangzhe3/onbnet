#pragma once

#include "base_worker.h"

class socket_worker: public onbnet::base_worker {
public:
    socket_worker(int max_connect, int port);
    ~socket_worker();

    virtual void start() override;
    virtual void stop() override;
    
private:
    bool is_stop_;
};