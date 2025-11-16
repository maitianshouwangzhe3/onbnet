

#include "connect_server.h"
#include "service_manager.h"
#include "net_worker_manager.h"

#include <cerrno>

connect_server::connect_server() {

}

connect_server::connect_server(std::shared_ptr<onbnet::net::socket> socket): socket_(socket) {
    socket_->set_fd_non_block();
}

connect_server::~connect_server() {

}

int connect_server::dispatch(onbnet::net::event& event) {
    switch (statu_) {
        case tatus::CONNECTING: {
            if (event.event_ & static_cast<uint32_t>(onbnet::net::event_type::EVENT_WRITE)) {
                statu_ = tatus::CONNECTED;

                auto S = net_worker_manager_inst->get_service(socket_->get_fd());
                if (S) {
                    std::shared_ptr<message> msg = std::make_shared<message>(sizeof(onbnet_socket_message));
                    msg->type = static_cast<int>(message_type::SOCKET);
                    onbnet_socket_message* data = static_cast<onbnet_socket_message*>(msg->data);
                    data->buffer = nullptr;
                    data->id = socket_->get_fd();
                    data->ud = 0;
                    data->type = static_cast<int>(message_type::SERVER_CONNECT);
                    msg->session = 0;
                    msg->source = 0;
                    service_manager_inst->send(S->service_id_, msg);
                }

                net_worker_manager_inst->event_mod_read(socket_->get_fd(), false);
            }
        }
        break;
        case tatus::CONNECTED: {
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

std::shared_ptr<onbnet::net::socket> connect_server::get_socket() {
    return socket_;
}

int connect_server::close() {
    return event_close_hander();
}

int connect_server::connect(const char* ip, int port) {
    statu_ = tatus::CONNECTING;
    int ret = socket_->connect(ip, port);
    if (ret == -1) {
        if (errno == EINPROGRESS) {
            ret = net_worker_manager_inst->event_write(socket_->get_fd());
        } else {
            statu_ = tatus::ERROR;
            return -1;
        }
    }

    return ret;
}

int connect_server::event_read_hander() {
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
        data->ud = nready;
        msg->session = S->service_id_;
        msg->source = S->service_id_;
        service_manager_inst->send(S->service_id_, msg);
    }

    return nready;
}

int connect_server::event_write_hander() {
    int fd = socket_->get_fd();
    (void)socket_->send();
    net_worker_manager_inst->event_mod_read(fd);
    return 0;
}

int connect_server::event_rdhup_hander() {
    return 0;
}

int connect_server::event_close_hander() {
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
        service_manager_inst->send(S->service_id_, msg);
    }
    return 0;
}