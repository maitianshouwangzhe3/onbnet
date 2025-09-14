#pragma once

#include <cstddef>
#include <cstdint>

enum class MessageType {
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
};

struct onbnet_socket_message {
	int type;
	int id;
	int ud;
	char * buffer;
};

class Message {
public:
    Message();
    Message(int size);
    Message(const Message& msg);
    ~Message();

    void operator=(const Message& msg);

    void* data;
    int type;
    int session;
    uint32_t source;
    size_t sz;
    bool is_free;
};