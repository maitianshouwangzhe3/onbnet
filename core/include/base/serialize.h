#pragma once

struct lua_State;

extern "C" { 
    int luaopen_serialize(lua_State *L);
}