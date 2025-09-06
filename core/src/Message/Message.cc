
#include "Message.h"
#include <cstdlib>
#include <cstring>

Message::Message() {
    data = nullptr;
    session = 0;
    source = 0;
    sz = 0;
    type = 0;
}

Message::Message(int size) {
    data = malloc(size);
    memset(data, 0, size);
    session = 0;
    source = 0;
    sz = size;
    type = 0;
}

Message::Message(const Message& msg) {
    data = malloc(msg.sz);
    memcpy(data, msg.data, msg.sz);
    session = msg.session;
    source = msg.source;
    sz = msg.sz;
    type = msg.type;
}

void Message::operator=(const Message& msg) {
    data = malloc(msg.sz);
    memcpy(data, msg.data, msg.sz);
    session = msg.session;
    source = msg.source;
    sz = msg.sz;
    type = msg.type;
}

Message::~Message() {
    if (data) {
        free(data);
        data = nullptr;
    }
}