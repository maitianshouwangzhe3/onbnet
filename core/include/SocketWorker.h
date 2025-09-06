#pragma once

#include "baseWorker.h"

class SocketWorker: public onbnet::baseWorker {
public:
    SocketWorker(int maxConnect, int port);
    ~SocketWorker();

    virtual void start() override;
    virtual void stop() override;
    
private:
    bool mIsStop;
};