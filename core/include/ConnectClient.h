#pragma once

#include "Socket.h"
#include "Connect.h"

class ConnectClient final: public Connect { 
public:
    ConnectClient();
    ConnectClient(int fd);
    ~ConnectClient();

    virtual int OnMessage(Event& event) override;

    char* RecvData();

    int SendData(char* data, int len);

    virtual std::shared_ptr<Socket> GetSocket() override;

private:
    int EventReadHander();

    int EventWriteHander();

    int EventRDHUPHander();

    int EventCloseHander();

private:
    std::shared_ptr<Socket> socket;
};