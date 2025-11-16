#pragma once

#include "socket.h"
#include <memory>


namespace onbnet::net {

struct event;

class connect {
public:
    virtual int dispatch(onbnet::net::event& event) = 0;
    virtual std::shared_ptr<socket> get_socket() = 0;
    virtual int close() = 0;
};

}
