

#include <cstdlib>
#include <cstring>
#include <memory>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lua-crypt.h"
}

#include "luna.h"
#include "timer.h"

#include "message.h"
#include "service.h"
#include "lua_mongo.h"
#include "serialize.h"
#include "lua_onbnet.h"
#include "lua_socket.h"
#include "lua_logger.h"
#include "service_manager.h"
#include "net_worker_manager.h"


#define REGISTER_LIBRARYS(name, lua_c_fn) \
    luaL_requiref(L, name, lua_c_fn, 0); \
    lua_pop(L, 1) /* remove lib */

static int get_obj_from_cpp(lua_State *L) {
    if (!service_manager_inst) {
        new service_manager();
    }

    lua_push_object(L, service_manager_inst);
    return 1;
}

static int lsend(lua_State *L) {
    auto serviceId = luaL_checkinteger(L, 1);
    auto type = luaL_checkinteger(L, 2);
    size_t len = 0;
    auto session = 0;

    if (lua_isnil(L, 3)) {
		session = 0;
	} else {
		session = luaL_checkinteger(L, 3);
        if (session == 0) {
            auto s = get_service(L);
            if (s) {
                session = s->get_session();
            }
        }
	}

    int mtype = lua_type(L,4);
    void* data = nullptr;
    bool is_free = true;
    switch (mtype) {
	case LUA_TSTRING: {
		data = (void*)lua_tolstring(L, 4, &len);
		if (len == 0) {
			data = nullptr;
		}
		break;
	}
	case LUA_TLIGHTUSERDATA: {
        data = lua_touserdata(L,4);
        is_free = false;
		break;
	}
	default:
		luaL_error(L, "invalid param %s", lua_typename(L, lua_type(L,4)));
	}

    std::shared_ptr<message> msg = nullptr;
    if (is_free) {
        msg = std::make_shared<message>(len);
        memcpy(msg->data, data, len);
    } else {
        msg = std::make_shared<message>();
        msg->data = data;
    }

    msg->is_free = is_free;
    msg->type = static_cast<int>(type);
    msg->session = session;
    msg->source = get_service(L)->service_id_;

    if (service_manager_inst->send(serviceId, msg)) {
        lua_pushinteger(L, session);
    } else {
        lua_pushinteger(L, -1);
    }

    return 1;
}

static int lcallback(lua_State *L) {
    auto s = get_service(L);
    s->set_callback();
    return 0;
}

static int lnoBlock(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = net_worker_manager_inst->get_connect(fd);
    auto socket = c->get_socket();
    socket->set_fd_non_block();
    return 0;
}

static int lself(lua_State *L) {
    lua_pushinteger(L, get_service(L)->service_id_);
    return 1;
}

static int lsleep(lua_State *L) {
    int session = -1;
    auto s = get_service(L);
    if (s) {
        session = s->get_session();
        int time = luaL_checkinteger(L, 1);
        int serviceId = s->service_id_;
        timer_inst->add_timer(time, [session, serviceId](){
            std::shared_ptr<message> msg = std::make_shared<message>();
            msg->type = static_cast<int>(message_type::RESPONSE);
            msg->session = session;
            service_manager_inst->send(serviceId, msg);
        });
    }

    lua_pushinteger(L, session);
    return 1;
}

static int lnew_session(lua_State *L) {
    int session = -1;
    auto s = get_service(L);
    if (s) {
        session = s->get_session();
    }

    lua_pushinteger(L, session);
    return 1;
}

extern "C" { 
LUAMOD_API int luaopen_onbnet_core(lua_State *L) {
	luaL_checkversion(L);
    
	luaL_Reg l[] = {
        {"get_obj_from_cpp", get_obj_from_cpp},
        {"send", lsend},
        {"callback", lcallback},
        {"noBlock", lnoBlock},
        {"self", lself},
        {"sleep", lsleep},
        {"new_session", lnew_session},
		{ NULL, NULL },
	};

    luaL_newlib(L, l);

	return 1;
}

void open_onbnet_libs(lua_State* L) {
    REGISTER_LIBRARYS("onbnet.core", luaopen_onbnet_core);

    REGISTER_LIBRARYS("onbnet.seri", luaopen_serialize);

    REGISTER_LIBRARYS("onbnet.socketdriver", luaopen_onbnet_socketdriver);

    REGISTER_LIBRARYS("onbnet.loggerdriver", luaopen_onbnet_loggerdriver);

    REGISTER_LIBRARYS("onbnet.crypt", luaopen_client_crypt);

    REGISTER_LIBRARYS("onbnet.mongodriver", luaopen_onbnet_mongo_driver);
}


}