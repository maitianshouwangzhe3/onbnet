#ifndef __LUA_CRYPT_H__
#define __LUA_CRYPT_H__


#include "lua.h"
#include "lauxlib.h"

LUAMOD_API int luaopen_skynet_crypt(lua_State *L);

LUAMOD_API int luaopen_client_crypt(lua_State *L);

#endif