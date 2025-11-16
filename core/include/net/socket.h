#pragma once

// #include "luna.h"
// #include "export.h"
#include "chain_buffer.h"

#include <memory>

namespace onbnet::net {

enum class protocol {
    TCP,
    UDP
};

class socket final{
public:
    socket();
    socket(int fd);
    socket(protocol proto);
    socket(int fd, protocol proto);
    ~socket();

    virtual bool bind(int port);

    virtual int listen();

    virtual void close();

    virtual void set_fd_non_block();

    virtual int recv();

    virtual int rbuffer_len();

    virtual int put(void* data, uint32_t datlen);

    virtual int send();

    virtual int wbuffer_len();

    virtual int push(void* data, uint32_t datlen);

    virtual int get_fd();

    virtual int accept();

    virtual int set_socket(int fd);

    virtual int create_socket();

    virtual int connect(const char* ip, int port);

    virtual int no_delay();

    virtual std::shared_ptr<onbnet::chain_buffer> get_rbuffer();

    virtual std::shared_ptr<onbnet::chain_buffer> get_wbuffer();

    virtual int check_sep_rbuffer(const char* sep, const int seplen);

    // DECLARE_LUA_CLASS(Socket);
    void init(protocol proto);
private:
    int fd_;
    protocol proto_;

    std::shared_ptr<onbnet::chain_buffer> rbuffer;
    std::shared_ptr<onbnet::chain_buffer> wbuffer;
};


}