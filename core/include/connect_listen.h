#pragma once
#include "socket.h"
#include "connect.h"

class connect_listen final: public onbnet::net::connect {
public:
    connect_listen(onbnet::net::protocol protocol = onbnet::net::protocol::TCP);

    ~connect_listen();

    virtual int dispatch(onbnet::net::event& event) override;

    virtual std::shared_ptr<onbnet::net::socket> get_socket() override;

    virtual int close() override;

private:
    int on_accept_message();

private:
    std::shared_ptr<onbnet::net::socket> socket_;
};