#pragma once

#include "socket.h"
#include "connect.h"

class connect_server final: public onbnet::net::connect {
public:
    connect_server();
    connect_server(std::shared_ptr<onbnet::net::socket> socket);
    ~connect_server();

    virtual int dispatch(onbnet::net::event& event) override;

    virtual std::shared_ptr<onbnet::net::socket> get_socket() override;

    virtual int close() override;

    int connect(const char* ip, int port);

private:
    int event_read_hander();

    int event_write_hander();

    int event_rdhup_hander();

    int event_close_hander();
private:
    enum class tatus {
        CONNECTING = 0,
        CONNECTED,
        ERROR,
    };

    tatus statu_;
    std::shared_ptr<onbnet::net::socket> socket_;
};