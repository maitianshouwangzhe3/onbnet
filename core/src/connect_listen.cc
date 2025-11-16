
#include "message.h"
#include "connect_listen.h"
#include "connect_client.h"
#include "net_worker_manager.h"
#include "service_manager.h"
#include "logger.h"
#include <iostream>
#include <memory>
connect_listen::connect_listen(onbnet::net::protocol protocol) {
    socket_ = std::make_shared<onbnet::net::socket>(protocol);
}

connect_listen::~connect_listen() {

}

int connect_listen::dispatch(onbnet::net::event& event) {
    if (event.event_ & static_cast<uint32_t>(onbnet::net::event_type::EVENT_READ)) {
        return on_accept_message();
    }
    return -1;
}

int connect_listen::on_accept_message() {
    int fd = socket_->accept();
    if (fd > 0) {
        std::shared_ptr<onbnet::net::connect> connect = std::make_shared<connect_client>(fd);
        if (connect) {;
            connect->get_socket()->set_fd_non_block();
            net_worker_manager_inst->add_connect(connect);
            auto S = net_worker_manager_inst->get_service(socket_->get_fd());
            if (S) {
                std::shared_ptr<message> msg = std::make_shared<message>(sizeof(onbnet_socket_message));
                msg->type = static_cast<int>(message_type::SOCKET);
                onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
                data->buffer = nullptr;
                data->id = socket_->get_fd();
                data->ud = fd;
                data->type = static_cast<int>(message_type::CONNECT);
                // msg->data = static_cast<void*>(data);
                msg->session = 0;
                msg->source = S->service_id_;
                service_manager_inst->send(msg);
            }
        } else {
            log_error("accept error");
        }
    }
    return 0;
}

std::shared_ptr<onbnet::net::socket> connect_listen::get_socket() {
    return socket_;
}

int connect_listen::close() {
    int fd = socket_->get_fd();
    return net_worker_manager_inst->delete_connect(fd);
}