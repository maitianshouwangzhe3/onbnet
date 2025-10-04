#pragma once

#include "Socket.h"
#include "Connect.h"

class ConnectServer final: public Connect {
public:
    ConnectServer();
    ConnectServer(std::shared_ptr<Socket> socket);
    ~ConnectServer();

    virtual int OnMessage(Event& event) override;

    virtual std::shared_ptr<Socket> GetSocket() override;

    virtual int Close() override;

    int Connect(const char* ip, int port);

private:
    int EventReadHander();

    int EventWriteHander();

    int EventRDHUPHander();

    int EventCloseHander();
private:
    enum class tatus {
        CONNECTING = 0,
        CONNECTED,
        ERROR,
    };

    tatus mStatus;
    std::shared_ptr<Socket> mSocket;
};