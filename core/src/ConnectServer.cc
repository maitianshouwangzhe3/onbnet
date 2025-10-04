
#include "ConnectServer.h"
#include "ServiceManager.h"
#include "NetWorkerManager.h"

#include <cerrno>

ConnectServer::ConnectServer() {

}

ConnectServer::ConnectServer(std::shared_ptr<Socket> socket): mSocket(socket) {
    mSocket->SetFdNonBlock();
}

ConnectServer::~ConnectServer() {

}

int ConnectServer::OnMessage(Event& event) {
    switch (mStatus) {
        case tatus::CONNECTING: {
            if (event.event & static_cast<uint32_t>(EventType::EVENT_WRITE)) {
                mStatus = tatus::CONNECTED;

                auto S = NetWorkerManagerInst->GetService(mSocket->GetFd());
                if (S) {
                    std::shared_ptr<Message> msg = std::make_shared<Message>(sizeof(onbnet_socket_message));
                    msg->type = static_cast<int>(MessageType::SOCKET);
                    onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
                    data->buffer = nullptr;
                    data->id = mSocket->GetFd();
                    data->ud = 0;
                    data->type = static_cast<int>(MessageType::SERVER_CONNECT);
                    msg->session = 0;
                    msg->source = 0;
                    ServiceManagerInst->Send(S->ServiceId, msg);
                }

                NetWorkerManagerInst->eventModRead(mSocket->GetFd(), false);
            }
        }
        break;
        case tatus::CONNECTED: {
            if (event.event & static_cast<uint32_t>(EventType::EVENT_RDHUP)) {
                return EventRDHUPHander();
            } else {
                if (event.event & static_cast<uint32_t>(EventType::EVENT_READ)) {
                    (void)EventReadHander();
                }

                if (event.event & static_cast<uint32_t>(EventType::EVENT_WRITE)) {
                    (void)EventWriteHander();
                }
            }
        }
        break;
        case tatus::ERROR: {

        }
        break;
        default: {

        }
    }
    return 0;
}

std::shared_ptr<Socket> ConnectServer::GetSocket() {
    return mSocket;
}

int ConnectServer::Close() {
    return EventCloseHander();
}

int ConnectServer::Connect(const char* ip, int port) {
    mStatus = tatus::CONNECTING;
    int ret = mSocket->Connect(ip, port);
    if (ret == -1) {
        if (errno == EINPROGRESS) {
            ret = NetWorkerManagerInst->eventWrite(mSocket->GetFd());
        } else {
            mStatus = tatus::ERROR;
            return -1;
        }
    }

    return ret;
}

int ConnectServer::EventReadHander() {
    int nready = mSocket->Recv();
    if (nready < 0) {
        return nready;
    } else if (nready == 0) {
        return EventCloseHander();
    }

    auto S = NetWorkerManagerInst->GetService(mSocket->GetFd());
    if (S) {
        std::shared_ptr<Message> msg = std::make_shared<Message>(sizeof(onbnet_socket_message));
        msg->type = static_cast<int>(MessageType::SOCKET);
        onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
        data->buffer = nullptr;
        data->id = mSocket->GetFd();
        data->type = static_cast<int>(MessageType::DATA);
        // msg->data = static_cast<void*>(data);
        msg->session = S->ServiceId;
        msg->source = S->ServiceId;
        // ServiceManagerInst->Send(msg);
        ServiceManagerInst->Send(S->ServiceId, msg);
    }

    return nready;
}

int ConnectServer::EventWriteHander() {
    int fd = mSocket->GetFd();
    (void)mSocket->Send();
    NetWorkerManagerInst->eventModRead(fd);
    return 0;
}

int ConnectServer::EventRDHUPHander() {
    return 0;
}

int ConnectServer::EventCloseHander() {
    int fd = mSocket->GetFd();
    NetWorkerManagerInst->DeleteConnect(fd);
    auto S = NetWorkerManagerInst->GetService(fd);
    if (S) {
        std::shared_ptr<Message> msg = std::make_shared<Message>(sizeof(onbnet_socket_message));
        msg->type = static_cast<int>(MessageType::SOCKET);
        onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
        data->buffer = nullptr;
        data->id = fd;
        data->type = static_cast<int>(MessageType::DISCONNECT);
        // msg->data = static_cast<void*>(data);
        msg->session = S->ServiceId;
        msg->source = S->ServiceId;
        ServiceManagerInst->Send(S->ServiceId, msg);
    }
    return 0;
}