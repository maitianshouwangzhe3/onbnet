
#include "Socket.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Socket::Socket() {
    Init(protocol::TCP);
    rBuffer = std::make_shared<onbnet::ChainBuffer>();
    wBuffer = std::make_shared<onbnet::ChainBuffer>();
}

Socket::~Socket() {

}

Socket::Socket(int fd): mFd(fd) {
    rBuffer = std::make_shared<onbnet::ChainBuffer>();
    wBuffer = std::make_shared<onbnet::ChainBuffer>();
}

Socket::Socket(protocol proto) {
    Init(proto);
    rBuffer = std::make_shared<onbnet::ChainBuffer>();
    wBuffer = std::make_shared<onbnet::ChainBuffer>();
}

Socket::Socket(int fd, protocol proto): mFd(fd) {
    Init(proto);
    rBuffer = std::make_shared<onbnet::ChainBuffer>();
    wBuffer = std::make_shared<onbnet::ChainBuffer>();
}

bool Socket::Bind(int port) {
    if (mFd <= 0) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return bind(mFd, (struct sockaddr *)&addr, sizeof(addr)) > 0;
}

int Socket::Listen() {
    if (mFd <= 0) {
        return false;
    }

    return listen(mFd, 5) == 0;
}

void Socket::Close() {
    if (mFd > 0) {
        close(mFd);
        mFd = -1;
    }
}

void Socket::SetFdNonBlock() {
    int flags = fcntl(mFd, F_GETFL, 0);
    fcntl(mFd, F_SETFL, flags | O_NONBLOCK);
}

int Socket::Recv() {
    int ret = 0;
    do {
        char buffer[4096] = {0};
        int nready = recv(mFd, buffer, 4096, 0);
        if (nready == 0) {
            return 0;
        } else if (nready < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK) {
                break;
            }
        } else {
            rBuffer->BufferAdd(buffer, nready);
            ret += nready;
        }
    } while (true);
    return ret;
}

int Socket::WBufferLen() {
    return wBuffer->BufferLen();
}

int Socket::RBufferLen() {
    return rBuffer->BufferLen();
}

int Socket::Put(void* data, uint32_t datlen) {
    return rBuffer->BufferRemove(data, datlen);
}

int Socket::Send() {
    int ret = 0;
    do {
        char buffer[4096] = {0};
        int len = wBuffer->BufferRemove(buffer, 4096);
        int size = 0;
        if (len == 4096) {
            size = send(mFd, buffer, len, 0);
        } else if (len < 4096 && len > 0) {
            size = send(mFd, buffer, len, 0);
            break;
        } else {
            break;
        }

        if (size < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK) {
                break;
            }
        }
        ret += size;
    } while (true);
    return ret;
}

int Socket::Push(void* data, uint32_t datlen) {
    return wBuffer->BufferAdd(data, datlen);
}

int Socket::GetFd() {
    return mFd;
}

int Socket::Accept() {
    return accept(mFd, NULL, NULL);
}

int Socket::SetSocket(int fd) {
    mFd = fd;
    return mFd;
}

int Socket::CreateSocket() {
    Init(mProto);
    return mFd;
}

void Socket::Init(protocol proto) {
    int type = -1;
    switch (proto) {
        case protocol::TCP:
            type = SOCK_STREAM;
            break;
        case protocol::UDP:
            type = SOCK_DGRAM;
            break;
        default:
            type = SOCK_STREAM;
            break;
    }

    mFd = socket(AF_INET, type, 0);
}

std::shared_ptr<onbnet::ChainBuffer> Socket::GetRBuffer() {
    return rBuffer;
}

std::shared_ptr<onbnet::ChainBuffer> Socket::GetWBuffer() {
    return wBuffer;
}