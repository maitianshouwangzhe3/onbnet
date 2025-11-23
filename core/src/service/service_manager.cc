
#include "util.h"
#include "service.h"
#include "logger.h"
#include "luna.h"
#include <atomic>
#include <cstring>
#include <stdexcept>
#include "service_manager.h"
#include "config_file_reader.h"


LUA_EXPORT_CLASS_BEGIN(service_manager)
LUA_EXPORT_METHOD(new_service)
LUA_EXPORT_CLASS_END()


service_manager* service_manager::inst = nullptr;

service_manager::service_manager() {
    if (!inst) {
        inst = this;
    } else {
        throw std::runtime_error("service_manager instance already exists");
    }
}

service_manager::~service_manager() {

}

void service_manager::init(onbnet::util::config_file_reader* config) {
    max_service_id = 0;
    service_vector_.reserve(24);
    for (int i = 0; i < 24; i++) {
        service_vector_[i] = nullptr;
    }

    char* tmp = config->get_config_name("lua_path");
    if (tmp) {
        lua_path_ = tmp;
    } else {
        throw std::runtime_error("lua_path not found");
    }
    
    tmp = config->get_config_name("lua_cpath");
    if (tmp) {
        cpath_ = tmp;
    } else {
        throw std::runtime_error("lua_cpath not found");
    }

    tmp = config->get_config_name("lua_service");
    if (tmp) {
        service_path_ = tmp;
    } else {
        throw std::runtime_error("lua_service not found");
    }

    tmp = config->get_config_name("lua_watch_dir");
    if (tmp && strlen(tmp) != 0) {
        script_dir_ = tmp;
        file_watch_ = std::make_unique<filewatch::FileWatch<std::string>>(script_dir_, [this](const std::string &path, const filewatch::Event change_type)
	{
		std::string script_name = onbnet::util::path_to_lua_script_name(path, script_dir_);
        if (script_name.empty()) {
			return;
		}

		std::lock_guard<std::mutex> lock{watch_mutex_};

		switch (change_type)
		{
		case filewatch::Event::added:
		case filewatch::Event::renamed_new:
		{
			break;
		}
		case filewatch::Event::removed:
		case filewatch::Event::renamed_old:
		{
			break;
		}
		case filewatch::Event::modified:
		{
            push_hotfix(script_name);
			break;
		}
		default:
			break;
		}
	});
    }
}

void service_manager::push_hotfix(std::string& name) {
    for (int i = 0; i < 24; ++i) {
        auto S = service_vector_[i];
        if (S) {
            std::shared_ptr<message> msg = std::make_shared<message>(name.size());
            msg->type = static_cast<int>(message_type::SNAX);
            msg->sz = name.size();
            memcpy(msg->data, name.c_str(), msg->sz);
            msg->session = S->service_id_;
            msg->source = S->service_id_;
            service_manager_inst->send(S->service_id_, msg);
        }
    }
}

void service_manager::__gc() {
    
}

void service_manager::update() {

}

void service_manager::close_service(uint32_t service_id) {
    service_vector_[service_id] = nullptr;
}

bool service_manager::send(std::shared_ptr<message> msg) {
    auto S = service_vector_[msg->source];
    if (S) {
        S->push_message(msg);
        queue->Push(S);
        return true;
    }
    return false;
}

bool service_manager::send(uint32_t serviceId, std::shared_ptr<message> msg) {
    auto S = service_vector_[serviceId];
    if (S) {
        S->push_message(msg);
        bool oldv = false;
        bool newv = true;
        if (S->Invoke_.compare_exchange_weak(oldv, newv, std::memory_order_release, std::memory_order_release)) {
            queue->Push(S);
        }
        
        return true;
    }
    return false;
}

int service_manager::new_service(const char* service_name) {
    service* s = new service(service_name);
    s->service_id_ = max_service_id++;
    s->lua_path_ = lua_path_;
    s->cpath_ = cpath_;
    s->service_path_ = service_path_;
    s->init();
    service_vector_[s->service_id_] = s;
    log_info("new service {} [{}]", service_name, s->service_id_);
    return s->service_id_;
}

void service_manager::set_queue(producer_consumer_queue<service*>* q) {
    queue = q;
}

void service_manager::push_global_msg_queue(service* s) {
    queue->Push(s);
}