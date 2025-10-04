
#include "NetWorkerManager.h"
#include "Poller.h"
#include <stdexcept>

NetWorkerManager* NetWorkerManager::inst = nullptr;

NetWorkerManager::NetWorkerManager(int maxConnect): mMaxConnect(maxConnect) {
    if (!inst) {
        inst = this;
    } else {
        throw std::runtime_error("NetWorkerManager::inst is not null");
    }

    mConnects.reserve(maxConnect + 1);
    mPoller = std::make_shared<Poller>();
}

NetWorkerManager::~NetWorkerManager() {

}

int NetWorkerManager::AddConnect(int fd) {
    return 0;
}

int NetWorkerManager::AddConnect(std::shared_ptr<Connect> connect, bool isEt) {
    int fd = connect->GetSocket()->GetFd();
    if (fd > mConnects.capacity()) {
        mConnects.reserve(fd + 1);
    }

    if (!mConnects[fd]) {
        mConnects[fd] = connect;
    }
    // mPoller->AddEventRead(fd, nullptr, isEt);
    return 0;
}

int NetWorkerManager::eventRead(int fd, bool isEt) {
    return mPoller->AddEventRead(fd, isEt);
}

int NetWorkerManager::eventWrite(int fd, bool isEt) {
    return mPoller->AddEventWrite(fd, isEt);
}

int NetWorkerManager::eventModReadWrite(int fd, bool isEt) {
    return mPoller->ModEventReadWrite(fd, isEt);
}

int NetWorkerManager::eventModRead(int fd, bool isEt) {
    return mPoller->ModEventRead(fd, isEt);
}

int NetWorkerManager::DeleteConnect(int fd) {
    mPoller->DelEvent(fd);
    mConnects[fd].reset();
    if (mService.find(fd) != mService.end()) {
        mService.erase(fd);
    }
    return 0;
}

std::shared_ptr<Connect> NetWorkerManager::GetConnect(int fd) {
    if(fd >= mMaxConnect || fd < 0) {
        return nullptr;
    }

    return mConnects[fd];
}

int NetWorkerManager::Poll(std::vector<Event>& events, int timeout) {
    return mPoller->Poll(events, timeout);
}

Service* NetWorkerManager::GetService(int id) {
    auto service = mService.find(id);
    if (service != mService.end()) {
        return service->second;
    }

    return nullptr;
}

void NetWorkerManager::SetService(int id, Service* service) {
    mService.insert(std::make_pair(id, service));
}