#pragma once
#include "Socket.h"
#include "Connect.h"

class ConnectListen final: public Connect {
public:
    ConnectListen(protocol protocol = protocol::TCP);

    ~ConnectListen();

    virtual int OnMessage(Event& event) override;

    virtual std::shared_ptr<Socket> GetSocket() override;

    virtual int Close() override;

private:
    int onAcceptMessage();

private:
    std::shared_ptr<Socket> socket;
};