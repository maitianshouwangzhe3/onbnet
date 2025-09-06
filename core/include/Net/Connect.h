#pragma once

#include "Socket.h"
#include <memory>

struct Event;

class Connect {
public:
    virtual int OnMessage(Event& event) = 0;
    virtual std::shared_ptr<Socket> GetSocket() = 0;
};