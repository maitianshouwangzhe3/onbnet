#pragma once

struct lua_State;

extern "C" {
    int luaopen_onbnet_loggerdriver(lua_State *L);
}

int llog_info(lua_State *L);
int lalog_info(lua_State *L);
int llog_error(lua_State *L);
int lalog_error(lua_State *L);
int llog_debug(lua_State *L);
int lalog_debug(lua_State *L);
int llog_debug(lua_State *L);
int lalog_debug(lua_State *L);
int llog_warn(lua_State *L);
int lalog_warn(lua_State *L);
int lconsole_info(lua_State *L);
int lconsole_debug(lua_State *L);
int lconsole_error(lua_State *L);
int lconsole_warn(lua_State *L);