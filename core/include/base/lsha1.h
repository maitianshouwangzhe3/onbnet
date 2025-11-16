#ifndef __LSHA1_H__
#define __LSHA1_H__ 

#include "lua.h"

// defined in lsha1.c
int lsha1(lua_State *L);
int lhmac_sha1(lua_State *L);

#endif