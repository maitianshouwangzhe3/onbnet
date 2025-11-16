

#include "service_manager.h"
#include "connect_client.h"
#include "message.h"
#include "net_worker_manager.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

connect_client::connect_client() {

}
connect_client::connect_client(int fd) {
    socket_ = std::make_shared<onbnet::net::socket>(fd);
}

connect_client::connect_client(std::shared_ptr<onbnet::net::socket> socket) {
    this->socket_ = socket;
}

connect_client::~connect_client() {

}

int connect_client::event_read_hander() {
    int nready = socket_->recv();
    if (nready < 0) {
        return nready;
    } else if (nready == 0) {
        return event_close_hander();
    }

    auto S = net_worker_manager_inst->get_service(socket_->get_fd());
    if (S) {
        std::shared_ptr<message> msg = std::make_shared<message>(sizeof(onbnet_socket_message));
        msg->type = static_cast<int>(message_type::SOCKET);
        onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
        data->buffer = nullptr;
        data->id = socket_->get_fd();
        data->type = static_cast<int>(message_type::DATA);
        // msg->data = static_cast<void*>(data);
        msg->session = S->service_id_;
        msg->source = S->service_id_;
        // ServiceManagerInst->Send(msg);
        service_manager_inst->send(S->service_id_, msg);
    }

    return nready;
}

int connect_client::event_write_hander() {
    int fd = socket_->get_fd();
    (void)socket_->send();
    net_worker_manager_inst->event_mod_read(fd);
    return 0;
}

int connect_client::event_rdhup_hander() {
    return 0;
}

int connect_client::event_close_hander() {
    int fd = socket_->get_fd();
    net_worker_manager_inst->delete_connect(fd);
    auto S = net_worker_manager_inst->get_service(fd);
    if (S) {
        std::shared_ptr<message> msg = std::make_shared<message>(sizeof(onbnet_socket_message));
        msg->type = static_cast<int>(message_type::SOCKET);
        onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
        data->buffer = nullptr;
        data->id = fd;
        data->type = static_cast<int>(message_type::DISCONNECT);
        // msg->data = static_cast<void*>(data);
        msg->session = S->service_id_;
        msg->source = S->service_id_;
        service_manager_inst->send(msg);
    }
    return 0;
}

int connect_client::dispatch(onbnet::net::event& event) {
    if (event.event_ & static_cast<uint32_t>(onbnet::net::event_type::EVENT_RDHUP)) {
        return event_rdhup_hander();
    } else {
        if (event.event_ & static_cast<uint32_t>(onbnet::net::event_type::EVENT_READ)) {
            (void)event_read_hander();
        }

        if (event.event_ & static_cast<uint32_t>(onbnet::net::event_type::EVENT_WRITE)) {
            (void)event_write_hander();
        }
    }

    return 0;
}

char* connect_client::recv_data() {
    int len = socket_->get_rbuffer()->buffer_len();
    char* data = (char*)malloc(len);
    memset(data, 0, len);
    socket_->get_rbuffer()->buffer_remove(data, len);
    return data;
}

int connect_client::send_data(char* data, int len) {
    return socket_->get_wbuffer()->buffer_add(data, len);
}

std::shared_ptr<onbnet::net::socket> connect_client::get_socket() {
    return socket_;
}

int connect_client::close() {
    return event_close_hander();
}