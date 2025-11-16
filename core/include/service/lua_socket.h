#pragma once

struct lua_State;

extern "C" {
    int luaopen_onbnet_socketdriver(lua_State *L);
}

int llisten(lua_State *L);
int lsocket_send(lua_State *L);
int lsocket_async_send(lua_State *L);
int lsocket_recv(lua_State *L);
int lsocket_start(lua_State *L);
int luaseri_socket_unpack(lua_State *L);
int lsocket_connect(lua_State *L);
int lsocket_nodelay(lua_State *L);
int lsocket_recvline(lua_State *L);
int lsocket_recvall(lua_State *L);
int lsocket_checkline(lua_State *L);
int lsocket_checkbufferlen(lua_State *L);

