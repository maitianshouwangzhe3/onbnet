
#include "Message.h"
#include <cstdlib>
#include <cstring>

Message::Message() {
    data = nullptr;
    session = 0;
    source = 0;
    sz = 0;
    type = 0;
    is_free = true;
}

Message::Message(int size) {
    data = malloc(size);
    memset(data, 0, size);
    session = 0;
    source = 0;
    sz = size;
    type = 0;
    is_free = true;
}

Message::Message(const Message& msg) {
    data = malloc(msg.sz);
    memcpy(data, msg.data, msg.sz);
    session = msg.session;
    source = msg.source;
    sz = msg.sz;
    type = msg.type;
    is_free = true;
}

void Message::operator=(const Message& msg) {
    data = malloc(msg.sz);
    memcpy(data, msg.data, msg.sz);
    session = msg.session;
    source = msg.source;
    sz = msg.sz;
    type = msg.type;
    is_free = true;
}

Message::~Message() {
    if (data && is_free) {
        free(data);
        data = nullptr;
    }
}