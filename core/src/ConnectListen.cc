
#include "Message.h"
#include "ConnectListen.h"
#include "ConnectClient.h"
#include "NetWorkerManager.h"
#include "ServiceManager.h"
#include <iostream>
#include <memory>
ConnectListen::ConnectListen(protocol protocol) {
    socket = std::make_shared<Socket>(protocol);
}

ConnectListen::~ConnectListen() {

}

int ConnectListen::OnMessage(Event& event) {
    if (event.event & static_cast<uint32_t>(EventType::EVENT_READ)) {
        return onAcceptMessage();
    }
    return -1;
}

int ConnectListen::onAcceptMessage() {
    int fd = socket->Accept();
    if (fd > 0) {
        std::shared_ptr<Connect> connect = std::make_shared<ConnectClient>(fd);
        if (connect) {;
            connect->GetSocket()->SetFdNonBlock();
            NetWorkerManagerInst->AddConnect(connect);
            auto S = NetWorkerManagerInst->GetService(socket->GetFd());
            if (S) {
                std::shared_ptr<Message> msg = std::make_shared<Message>(sizeof(onbnet_socket_message));
                msg->type = static_cast<int>(MessageType::SOCKET);
                onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
                data->buffer = nullptr;
                data->id = socket->GetFd();
                data->ud = fd;
                data->type = static_cast<int>(MessageType::CONNECT);
                // msg->data = static_cast<void*>(data);
                msg->session = 0;
                msg->source = S->ServiceId;
                ServiceManagerInst->Send(msg);
            }
        } else {
            std::cout << "accept error" << std::endl;
        }
    }
    return 0;
}

std::shared_ptr<Socket> ConnectListen::GetSocket() {
    return socket;
}

int ConnectListen::Close() {
    int fd = socket->GetFd();
    return NetWorkerManagerInst->DeleteConnect(fd);
}