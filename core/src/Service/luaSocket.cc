
#include "luaSocket.h"
#include "ConnectListen.h"
#include "ConnectServer.h"
#include "NetWorkerManager.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

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

int llisten(lua_State *L) {
    int port = luaL_checkinteger(L, 1);
    std::shared_ptr<Connect> cl = std::make_shared<ConnectListen>();
    auto socket = cl->GetSocket();
    socket->Bind(port);
    socket->Listen();
    NetWorkerManagerInst->AddConnect(cl);
    lua_pushinteger(L, socket->GetFd());
    return 1;
}

int lsocket_send(lua_State *L) {
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

int lsocket_async_send(lua_State *L) { 
    int fd = luaL_checkinteger(L, 1);
    const char* data = luaL_checkstring(L, 2);
    int len = luaL_checkinteger(L, 3);
    auto c = NetWorkerManagerInst->GetConnect(fd);
    auto socket = c->GetSocket();
    socket->Push((void*)data, len);
    NetWorkerManagerInst->eventModReadWrite(fd);
    return 0;
}

int lsocket_recv(lua_State *L) {
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

int lsocket_start(lua_State *L) {
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

int lsocket_connect(lua_State *L) {
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

int lsocket_nodelay(lua_State *L) {
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

int lsocket_recvline(lua_State *L) {
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

int lsocket_checkline(lua_State *L) {
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

int lsocket_checkbufferlen(lua_State *L) {
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

int lsocket_recvall(lua_State *L) {
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

int luaseri_socket_unpack(lua_State *L) {
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

int luaopen_onbnet_socketdriver(lua_State *L) {
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