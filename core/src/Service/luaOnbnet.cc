

#include "ConnectClient.h"
#include "ConnectListen.h"
#include "Message.h"
#include "NetWorkerManager.h"
#include "Socket.h"
#include <cstdlib>
#include <cstring>
#include <memory>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "luna.h"
#include "Timer.h"
#include <iostream>
#include "logger.h"
#include "Service.h"
#include "serialize.h"
#include "luaOnbnet.h"
#include "ServiceManager.h"
// #include <iostream>

#define REGISTER_LIBRARYS(name, lua_c_fn) \
    luaL_requiref(L, name, lua_c_fn, 0); \
    lua_pop(L, 1) /* remove lib */

static int get_obj_from_cpp(lua_State *L) {
    if (!ServiceManager::inst) {
        new ServiceManager();
    }

    lua_push_object(L, ServiceManager::inst);
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
            auto s = GetService(L);
            if (s) {
                session = s->GetSession();
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

    std::shared_ptr<Message> msg = nullptr;
    if (is_free) {
        msg = std::make_shared<Message>(len);
        memcpy(msg->data, data, len);
    } else {
        msg = std::make_shared<Message>();
        msg->data = data;
    }

    msg->is_free = is_free;
    msg->type = static_cast<int>(type);
    msg->session = session;
    msg->source = GetService(L)->ServiceId;

    if (ServiceManagerInst->Send(serviceId, msg)) {
        lua_pushinteger(L, session);
    } else {
        lua_pushinteger(L, -1);
    }

    return 1;
}

static int lcallback(lua_State *L) {
    auto s = GetService(L);
    s->setCallBack();
    return 0;
}

static int llisten(lua_State *L) {
    int port = luaL_checkinteger(L, 1);
    std::shared_ptr<Connect> cl = std::make_shared<ConnectListen>();
    auto socket = cl->GetSocket();
    socket->Bind(port);
    socket->Listen();
    NetWorkerManagerInst->AddConnect(cl);
    lua_pushinteger(L, socket->GetFd());
    return 1;
}

static int lsocket_send(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    const char* data = luaL_checkstring(L, 2);
    int len = luaL_checkinteger(L, 3);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    auto socket = c->GetSocket();
    socket->Push((void*)data, len);
    socket->Send();
    lua_pushinteger(L, len);
    return 1;
}

static int lsocket_async_send(lua_State *L) { 
    int fd = luaL_checkinteger(L, 1);
    const char* data = luaL_checkstring(L, 2);
    int len = luaL_checkinteger(L, 3);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    auto socket = c->GetSocket();
    socket->Push((void*)data, len);
    NetWorkerManagerInst->eventModReadWrite(fd);
    return 0;
}

static int lsocket_recv(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    auto socket = c->GetSocket();
    // socket->Recv();
    int len = socket->RBufferLen();
    char* data = (char*)malloc(len);
    socket->Put(static_cast<void*>(data), len);
    
    lua_pushlstring(L, data, len);
    lua_pushinteger(L, len);
    return 2;
}

static int lsocket_start(lua_State *L) {
    auto s = GetService(L);
    int fd = luaL_checkinteger(L, 1);
    int block = luaL_checkinteger(L, 2);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    if (c) {
        auto socket = c->GetSocket();
        NetWorkerManagerInst->SetService(socket->GetFd(), s);
        NetWorkerManagerInst->eventRead(fd, block != 0 ? false : true);
    } else {
        luaL_error(L, "invalid param %d", fd);
    }
    return 0;
}

static int lsocket_connect(lua_State *L) {
    const char* host = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);
    int ret = -1;
    if (host) {
        auto socketObj = std::make_shared<Socket>(protocol::TCP);
        ret = socketObj->Connect(host, port);
        if (ret == 0) {
            std::shared_ptr<Connect> connect = std::make_shared<ConnectClient>(socketObj);
            NetWorkerManagerInst->AddConnect(connect);
            ret = socketObj->GetFd();
        }
    }
    
    lua_pushinteger(L, ret);
    return 1;
}

static int lsocket_nodelay(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    int ret = -1;
    auto c = NetWorkerManagerInst->GetConnect(fd);
    if (c) {
        auto socket = c->GetSocket();
        ret = socket->noDelay();
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int lnoBlock(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    auto socket = c->GetSocket();
    socket->SetFdNonBlock();
    return 0;
}

static int lself(lua_State *L) {
    lua_pushinteger(L, GetService(L)->ServiceId);
    return 1;
}

static int lsleep(lua_State *L) {
    int session = -1;
    auto s = GetService(L);
    if (s) {
        session = s->GetSession();
        int time = luaL_checkinteger(L, 1);
        int serviceId = s->ServiceId;
        TimerInst->addTimer(time, [session, serviceId](){
            std::shared_ptr<Message> msg = std::make_shared<Message>();
            msg->type = static_cast<int>(MessageType::RESPONSE);
            msg->session = session;
            std::cout << "sleep callback" << std::endl;
            ServiceManagerInst->Send(serviceId, msg);
        });
    }

    lua_pushinteger(L, session);
    return 1;
}

static int lnew_session(lua_State *L) {
    int session = -1;
    auto s = GetService(L);
    if (s) {
        session = s->GetSession();
    }

    lua_pushinteger(L, session);
    return 1;
}

static int luaseri_socket_unpack(lua_State *L) {
	onbnet_socket_message* message = (onbnet_socket_message*)lua_touserdata(L,1);
	(void)luaL_checkinteger(L,2);

	lua_pushinteger(L, message->type);
	lua_pushinteger(L, message->id);
	lua_pushinteger(L, message->ud);
	if (message->buffer == NULL) {
		lua_pushlstring(L, "null", 4);
	} else {
		lua_pushlightuserdata(L, message->buffer);
	}
	return 4;
}


static int llog_info(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_info(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lalog_info(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_info(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int llog_error(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_error(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lalog_error(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_error(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int llog_debug(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_debug(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lalog_debug(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_debug(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int llog_warn(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_warn(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lalog_warn(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_warn(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lconsole_info(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_info(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lconsole_error(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_error(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lconsole_debug(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_debug(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

static int lconsole_warn(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_warn(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
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

LUAMOD_API int luaopen_onbnet_socketdriver(lua_State *L) {
	luaL_checkversion(L);
    
	luaL_Reg l[] = {
        {"listen", llisten},
        {"send", lsocket_send},
        {"async_send", lsocket_async_send},
        {"recv", lsocket_recv},
        {"start", lsocket_start},
        {"unpack", luaseri_socket_unpack},
        {"connect", lsocket_connect},
        {"nodelay", lsocket_nodelay},
		{ NULL, NULL },
	};

    luaL_newlib(L, l);

	return 1;
}

LUAMOD_API int luaopen_onbnet_loggerdriver(lua_State *L) {
	luaL_checkversion(L);
    
	luaL_Reg l[] = {
        {"log_info", llog_info},
        {"alog_info", lalog_info},
        {"log_error", llog_error},
        {"alog_error", lalog_error},
        {"log_debug", llog_debug},
        {"alog_debug", lalog_debug},
        {"log_debug", llog_debug},
        {"alog_debug", lalog_debug},
        {"log_warn", llog_warn},
        {"alog_warn", lalog_warn},
        {"console_info", lconsole_info},
        {"console_debug", lconsole_debug},
        {"console_error", lconsole_error},
        {"console_warn", lconsole_warn},
		{ NULL, NULL },
	};

    luaL_newlib(L, l);

	return 1;
}


void openOnbnetLibs(lua_State* L) {
    REGISTER_LIBRARYS("onbnet.core", luaopen_onbnet_core);

    REGISTER_LIBRARYS("onbnet.seri", luaopen_serialize);

    REGISTER_LIBRARYS("onbnet.socketdriver", luaopen_onbnet_socketdriver);

    REGISTER_LIBRARYS("onbnet.loggerdriver", luaopen_onbnet_loggerdriver);
}


}