
#include "message.h"
#include <cstdlib>
#include <cstring>

message::message() {
    data = nullptr;
    session = 0;
    source = 0;
    sz = 0;
    type = 0;
    is_free = true;
}

message::message(int size) {
    data = malloc(size);
    memset(data, 0, size);
    session = 0;
    source = 0;
    sz = size;
    type = 0;
    is_free = true;
}

message::message(const message& msg) {
    data = malloc(msg.sz);
    memcpy(data, msg.data, msg.sz);
    session = msg.session;
    source = msg.source;
    sz = msg.sz;
    type = msg.type;
    is_free = true;
}

void message::operator=(const message& msg) {
    data = malloc(msg.sz);
    memcpy(data, msg.data, msg.sz);
    session = msg.session;
    source = msg.source;
    sz = msg.sz;
    type = msg.type;
    is_free = true;
}

message::~message() {
    if (data && is_free) {
        free(data);
        data = nullptr;
    }
}