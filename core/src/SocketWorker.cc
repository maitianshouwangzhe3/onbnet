
#include <sys/socket.h>
#include <iostream>
#include "SocketWorker.h"
#include "NetWorkerManager.h"
SocketWorker::SocketWorker(int maxConnect, int port): mIsStop(false) {
    // std::shared_ptr<ConnectListen> connect = std::make_shared<ConnectListen>();
    // connect->GetSocket()->Bind(port);
    // connect->GetSocket()->Listen();

    new NetWorkerManager(maxConnect);
    // NetWorkerManagerInst->AddConnect(connect);
}

SocketWorker::~SocketWorker() {

}

void SocketWorker::start() {
    while (!mIsStop) {
        std::vector<Event> events;
        int ret = NetWorkerManagerInst->Poll(events, 50);
        if (ret < 0) {
            continue;
        }

        for (auto &event: events) {
            auto connect = NetWorkerManagerInst->GetConnect(event.fd);
            if (connect) {
                connect->OnMessage(event);
            }
        }
    }
}

void SocketWorker::stop() {
    mIsStop = true;
}