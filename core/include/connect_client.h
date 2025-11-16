#pragma once

#include "socket.h"
#include "connect.h"

class connect_client final: public onbnet::net::connect { 
public:
    connect_client();
    connect_client(int fd);
    connect_client(std::shared_ptr<onbnet::net::socket> socket);
    ~connect_client();

    virtual int dispatch(onbnet::net::event& event) override;

    char* recv_data();

    int send_data(char* data, int len);

    virtual std::shared_ptr<onbnet::net::socket> get_socket() override;

    virtual int close() override;

private:
    int event_read_hander();

    int event_write_hander();

    int event_rdhup_hander();

    int event_close_hander();

private:
    std::shared_ptr<onbnet::net::socket> socket_;
};