

#include "ConnectClient.h"
#include "ConnectListen.h"
#include "ConnectServer.h"
#include "Message.h"
#include "NetWorkerManager.h"
#include "Socket.h"
#include "timewheel.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lua-crypt.h"
#include "luasocket.h"
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

static size_t count_size(lua_State *L, int index) {
	size_t tlen = 0;
	int i;
	for (i=1;lua_geti(L, index, i) != LUA_TNIL; ++i) {
		size_t len;
		luaL_checklstring(L, -1, &len);
		tlen += len;
		lua_pop(L,1);
	}
	lua_pop(L,1);
	return tlen;
}

static void concat_table(lua_State *L, int index, void *buffer, size_t tlen) {
	char* ptr = (char*)buffer;
	int i = 0;
	for (i=1;lua_geti(L, index, i) != LUA_TNIL; ++i) {
		size_t len;
		const char * str = lua_tolstring(L, -1, &len);
		if (str == NULL || tlen < len) {
			break;
		}
		memcpy(ptr, str, len);
		ptr += len;
		tlen -= len;
		lua_pop(L,1);
	}
	if (tlen != 0) {
		free(buffer);
		luaL_error(L, "Invalid strings table");
	}
	lua_pop(L,1);
}

static void* get_buffer(lua_State* L, int index, size_t* sz) {
	void* buffer = nullptr;
	switch(lua_type(L, index)) {
		size_t len;
	case LUA_TUSERDATA:
		// lua full useobject must be a raw pointer, it can't be a socket object or a memory object.
		buffer = lua_touserdata(L, index);
		if (lua_isinteger(L, index+1)) {
			*sz = lua_tointeger(L, index+1);
		} else {
			*sz = lua_rawlen(L, index);
		}
		break;
	case LUA_TLIGHTUSERDATA: {
		if (lua_isinteger(L, index + 1)) {
			*sz = (size_t)lua_tointeger(L,index+1);
		}

		buffer = lua_touserdata(L,index);
		break;
		}
	case LUA_TTABLE:
		// concat the table as a string
		len = count_size(L, index);
		buffer = malloc(len);
		concat_table(L, index, buffer, len);
		*sz = len;
		break;
	default:
		buffer = (void*)luaL_checklstring(L, index, sz);
		break;
	}

    return buffer;
}

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
    void* data = nullptr;
    int nargs = lua_gettop(L);
    size_t len = 0;
    if (nargs > 2) {
        len = luaL_checkinteger(L, 3);
    }

    data = get_buffer(L, 2, &len);
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
    if (c) {
        auto socket = c->GetSocket();
        int len = 0;
        if (lua_isnil(L, 2)) {
            len = socket->RBufferLen();
        } else {
            if(lua_isinteger(L, 2)) {
                len = luaL_checkinteger(L, 2);
            } else {
                len = socket->RBufferLen();
            }
        }

        char* data = (char*)malloc(len);
        int ret = socket->Put(static_cast<void*>(data), len);
        if (ret > 0) {
            lua_pushlstring(L, data, len);
            lua_pushinteger(L, len);
            return 2;
        }
        
        lua_pushnil(L);
        return 1;
    }
    
    lua_pushnil(L);
    lua_pushinteger(L, 0);
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
        if (socketObj) {
            auto s = GetService(L);
            if (s) {
                NetWorkerManagerInst->SetService(socketObj->GetFd(), s);
                std::shared_ptr<ConnectServer> connect = std::make_shared<ConnectServer>(socketObj);
                NetWorkerManagerInst->AddConnect(connect);
                ret = connect->Connect(host, port);
                if (ret == 0) {
                    ret = socketObj->GetFd();
                }
            }
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

static int lsocket_recvline(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    if (c) {
        auto socket = c->GetSocket();
        if (socket && socket->RBufferLen() > 0) {
            size_t seplen = 0;
            const char* sep = luaL_checklstring(L,2,&seplen);
            int datalen = socket->CheckSepRBuffer(sep, seplen);
            if (datalen > 0) { 
                char* data = (char*)malloc(datalen);
                socket->Put(static_cast<void*>(data), datalen);
                lua_pushlstring(L, data, datalen - seplen);
            } else {
                lua_pushboolean(L, false);
            }
        } else {
            lua_pushboolean(L, false);
        }
    }
    return 1;
}

static int lsocket_checkline(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    if (c) {
        auto socket = c->GetSocket();
        if (socket && socket->RBufferLen() > 0) {
            size_t seplen = 0;
            const char* sep = luaL_checklstring(L,2,&seplen);
            int datalen = socket->CheckSepRBuffer(sep, seplen);
            if (datalen > 0) { 
                lua_pushboolean(L, true);
                return 1;
            }
        }
    }

    lua_pushboolean(L, false);
    return 1;
}

static int lsocket_checkbufferlen(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    if (c) {
        auto socket = c->GetSocket();
        if (socket) {
            int len = socket->RBufferLen();
            lua_pushinteger(L, len);
            return 1;
        }
    }

    lua_pushboolean(L, 0);
    return 1;
}

static int lsocket_recvall(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    auto socket = c->GetSocket();
    int len = socket->RBufferLen();
    char* data = (char*)malloc(len);
    socket->Put(static_cast<void*>(data), len);
    
    lua_pushlstring(L, data, len);
    lua_pushinteger(L, len);
    return 2;
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
        {"recvline", lsocket_recvline},
        {"recvall", lsocket_recvall},
        {"checkline", lsocket_checkline},
        {"checkbufferlen", lsocket_checkbufferlen},
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

    REGISTER_LIBRARYS("socket.core", luaopen_socket_core);

    REGISTER_LIBRARYS("onbnet.crypt", luaopen_client_crypt);
}


}