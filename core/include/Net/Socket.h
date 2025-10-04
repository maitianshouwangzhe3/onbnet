#pragma once

// #include "luna.h"
// #include "export.h"
#include "ChainBuffer.h"

#include <memory>

enum class protocol {
    TCP,
    UDP
};
class Socket final{
public:
    Socket();
    Socket(int fd);
    Socket(protocol proto);
    Socket(int fd, protocol proto);
    ~Socket();

    virtual bool Bind(int port);

    virtual int Listen();

    virtual void Close();

    virtual void SetFdNonBlock();

    virtual int Recv();

    virtual int RBufferLen();

    virtual int Put(void* data, uint32_t datlen);

    virtual int Send();

    virtual int WBufferLen();

    virtual int Push(void* data, uint32_t datlen);

    virtual int GetFd();

    virtual int Accept();

    virtual int SetSocket(int fd);

    virtual int CreateSocket();

    virtual int Connect(const char* ip, int port);

    virtual int noDelay();

    virtual std::shared_ptr<onbnet::ChainBuffer> GetRBuffer();

    virtual std::shared_ptr<onbnet::ChainBuffer> GetWBuffer();

    virtual int CheckSepRBuffer(const char* sep, const int seplen);

    // DECLARE_LUA_CLASS(Socket);
    void Init(protocol proto);
private:
    int mFd;
    protocol mProto;

    std::shared_ptr<onbnet::ChainBuffer> rBuffer;
    std::shared_ptr<onbnet::ChainBuffer> wBuffer;
};