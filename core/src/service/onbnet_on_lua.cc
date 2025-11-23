extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
	#include "jemalloc.h"
}

#include "logger.h"
#include "message.h"
#include "lua_onbnet.h"
#include "onbnet_on_lua.h"

#include <ctime>
#include <cstdio>
#include <memory>
#include <cassert>

#define malloc je_malloc
#define calloc je_calloc
#define realloc je_realloc
#define free je_free

struct lstate {
    size_t mem;
    size_t mem_level;
};

#define NANOSEC 1000000000
#define MICROSEC 1000000
const size_t MEMLVL = 20971520; // 20M
const size_t MEM_1MB = 1048576; // 1M

static void* lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    lstate *s = (lstate*)ud;
    s->mem += nsize;
    if (ptr) s->mem -= osize;
    if (s->mem > s->mem_level) {
        do {
            s->mem_level += MEMLVL;
        } while (s->mem > s->mem_level);
        
        printf("luajit vm now use %.2f M's memory up\n", (float)s->mem / MEM_1MB);
    } else if (s->mem < s->mem_level - MEMLVL) {
        do {
            s->mem_level -= MEMLVL;
        } while (s->mem < s->mem_level);
        
        printf("luajit vm now use %.2f M's memory down\n", (float)s->mem / MEM_1MB);
    }
    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else {
        return realloc(ptr, nsize);
    }
}

static int traceback (lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else {
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

onbnet_on_lua::onbnet_on_lua(): lua_state_(nullptr), cb_context_(nullptr) {

}

onbnet_on_lua::~onbnet_on_lua() {

}

int onbnet_on_lua::operator()(std::shared_ptr<message>& msg) {
	lua_State *L = cb_context_->L;
	int trace = 1;
	int r;
	lua_pushvalue(L,2);

	lua_pushinteger(L, msg->type);
	lua_pushlightuserdata(L, const_cast<void*>(msg->data));
	lua_pushinteger(L,msg->sz);
	lua_pushinteger(L, msg->session);
	lua_pushinteger(L, msg->source);

	r = lua_pcall(L, 5, 0 , trace);
	if (r == LUA_OK) {
		return 0;
	}

	log_error("xpcall: {}", lua_tostring(L, -1));
	lua_pop(L,1);
	return -1;
}

int onbnet_on_lua::init(void* ctx, std::string file_name) { 
    lua_State *L = lua_state_;
	lua_gc(L, LUA_GCSTOP, 0);
	lua_pushboolean(L, 1);
	lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
	luaL_openlibs(L);
	open_onbnet_libs(L);

	lua_pushlightuserdata(L, ctx);
	lua_setfield(L, LUA_REGISTRYINDEX, "OnbnetContext");
	// lua_pop(L,1);

	lua_gc(L, LUA_GCGEN, 0, 0);

	lua_pushstring(L, lua_path_.c_str());
	lua_setglobal(L, "LUA_PATH");
	lua_pushstring(L, cpath_.c_str());
	lua_setglobal(L, "LUA_CPATH");
	lua_pushstring(L, service_path_.c_str());
	lua_setglobal(L, "LUA_SERVICE");

	lua_pushcfunction(L, traceback);
	assert(lua_gettop(L) == 1);

	const char * loader = "./lualib/loader.lua";
	int r = luaL_loadfile(L,loader);
	if (r != LUA_OK) {
		return 1;
	}
	lua_pushlstring(L, file_name.c_str(), file_name.size());
	r = lua_pcall(L,1,0,1);
	if (r != LUA_OK) {
		lua_pop(L, 1);
		printf("error: %s\n", lua_tostring(L, -1));
		return 1;
	}

	lua_settop(L,0);
	// if (lua_getfield(L, LUA_REGISTRYINDEX, "memlimit") == LUA_TNUMBER) {

	// }
	// lua_pop(L, 1);

	lua_gc(L, LUA_GCRESTART, 0);
	return 0;
}

void onbnet_on_lua::callback() {
    luaL_checktype(lua_state_,1,LUA_TFUNCTION);
	lua_settop(lua_state_,1);
	cb_context_ = (callback_context*)lua_newuserdatauv(lua_state_, sizeof(*cb_context_), 2);
	cb_context_->L = lua_newthread(lua_state_);
	lua_pushcfunction(cb_context_->L, traceback);
	lua_setiuservalue(lua_state_, -2, 1);
	lua_getfield(lua_state_, LUA_REGISTRYINDEX, "CallbackContext");
	lua_setiuservalue(lua_state_, -2, 2);
	lua_setfield(lua_state_, LUA_REGISTRYINDEX, "CallbackContext");
	lua_xmove(lua_state_, cb_context_->L, 1);
}

onbnet_on_lua* new_onbnet_on_lua() {
    onbnet_on_lua* luaL = new onbnet_on_lua();
    lstate* ud = new lstate;
    ud->mem = 0;
    ud->mem_level = MEMLVL;
    luaL->lua_state_ = lua_newstate(lua_alloc, ud);
    return luaL;
}