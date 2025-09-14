

#include "ConnectListen.h"
#include "Message.h"
#include "NetWorkerManager.h"
#include <cstdlib>
#include <cstring>
#include <memory>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "luna.h"
#include <iostream>
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

static int lnoBlock(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    auto socket = c->GetSocket();
    socket->SetFdNonBlock();
    return 0;
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


extern "C" { 
LUAMOD_API int luaopen_onbnet_core(lua_State *L) {
	luaL_checkversion(L);
    
	luaL_Reg l[] = {
        {"get_obj_from_cpp", get_obj_from_cpp},
        {"send", lsend},
        {"callback", lcallback},
        {"noBlock", lnoBlock},
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
		{ NULL, NULL },
	};

    luaL_newlib(L, l);

	return 1;
}


void openOnbnetLibs(lua_State* L) {
    REGISTER_LIBRARYS("onbnet.core", luaopen_onbnet_core);

    REGISTER_LIBRARYS("onbnet.seri", luaopen_serialize);

    REGISTER_LIBRARYS("onbnet.socketdriver", luaopen_onbnet_socketdriver);
}


}