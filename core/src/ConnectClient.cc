

#include "ServiceManager.h"
#include "ConnectClient.h"
#include "Message.h"
#include "NetWorkerManager.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

// LUA_EXPORT_CLASS_BEGIN(ConnectClient)
// LUA_EXPORT_METHOD(RecvData)
// LUA_EXPORT_METHOD(SendData)
// LUA_EXPORT_CLASS_END()

ConnectClient::ConnectClient() {

}
ConnectClient::ConnectClient(int fd) {
    socket = std::make_shared<Socket>(fd);
}

ConnectClient::~ConnectClient() {

}

int ConnectClient::EventReadHander() {
    int nready = socket->Recv();
    if (nready < 0) {
        return nready;
    } else if (nready == 0) {
        return EventCloseHander();
    }

    auto S = NetWorkerManagerInst->GetService(socket->GetFd());
    if (S) {
        std::shared_ptr<Message> msg = std::make_shared<Message>(sizeof(onbnet_socket_message));
        msg->type = static_cast<int>(MessageType::SOCKET);
        onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
        data->buffer = nullptr;
        data->id = socket->GetFd();
        data->type = static_cast<int>(MessageType::DATA);
        msg->data = static_cast<void*>(data);
        msg->session = S->ServiceId;
        ServiceManagerInst->Send(msg);
    }

    return nready;
}

int ConnectClient::EventWriteHander() {
    return 0;
}

int ConnectClient::EventRDHUPHander() {
    return 0;
}

int ConnectClient::EventCloseHander() {
    NetWorkerManagerInst->DeleteConnect(socket->GetFd());
    auto S = NetWorkerManagerInst->GetService(socket->GetFd());
    if (S) {
        std::shared_ptr<Message> msg = std::make_shared<Message>(sizeof(onbnet_socket_message));
        msg->type = static_cast<int>(MessageType::SOCKET);
        onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
        data->buffer = nullptr;
        data->id = socket->GetFd();
        data->type = static_cast<int>(MessageType::DISCONNECT);
        msg->data = static_cast<void*>(data);
        msg->session = S->ServiceId;
        ServiceManagerInst->Send(msg);
    }
    return 0;
}

int ConnectClient::OnMessage(Event& event) {
    switch (event.event) {
        case EventType::EVENT_READ: {
            return EventReadHander();
        }
        case EventType::EVENT_WRITE: {
            return EventWriteHander();
        }
        case EventType::EVENT_RDHUP: {
            return EventRDHUPHander();
        }
        default: {
            
        }
    }

    return 0;
}

char* ConnectClient::RecvData() {
    int len = socket->GetRBuffer()->BufferLen();
    char* data = (char*)malloc(len);
    memset(data, 0, len);
    socket->GetRBuffer()->BufferRemove(data, len);
    return data;
}

int ConnectClient::SendData(char* data, int len) {
    return socket->GetWBuffer()->BufferAdd(data, len);
}

std::shared_ptr<Socket> ConnectClient::GetSocket() {
    return socket;
}