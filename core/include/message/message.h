#pragma once

#include <cstddef>
#include <cstdint>

enum class message_type {
    TEXT,
    RESPONSE,
    MULTICAST,
    CLIENT,
    SYSTEM,
    HARBOR,
    SOCKET,
    ERROR,
    QUEUE,
    DEBUG,
    LUA,
    SNAX,
    CONNECT,
    DATA,
    DISCONNECT,
    SERVER_CONNECT,
};

struct onbnet_socket_message {
	int type;
	int id;
	int ud;
	char * buffer;
};

class message {
public:
    message();
    message(int size);
    message(const message& msg);
    ~message();

    void operator=(const message& msg);

    void* data;
    int type;
    int session;
    uint32_t source;
    size_t sz;
    bool is_free;
};