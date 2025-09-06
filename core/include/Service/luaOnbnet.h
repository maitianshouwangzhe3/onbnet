#pragma once

#include "lua.h"

extern "C" { 
    void openOnbnetLibs(lua_State* L);

    int luaopen_serialize(lua_State *L);
}