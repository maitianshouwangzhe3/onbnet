#pragma once

struct lua_State;

extern "C" { 
    int luaopen_onbnet_mongo_driver(lua_State *L);
}