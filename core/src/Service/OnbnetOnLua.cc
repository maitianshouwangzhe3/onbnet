
#include "Message.h"
#include <memory>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "luaOnbnet.h"
#include "OnbnetOnLua.h"

#include <ctime>
#include <cassert>
#include <cstdio>

extern "C" { 
    #include "jemalloc.h"
}

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

// static int timing_enable(lua_State *L, int co_index, lua_Number *start_time) {
// 	lua_pushvalue(L, co_index);
// 	lua_rawget(L, lua_upvalueindex(1));
// 	if (lua_isnil(L, -1)) {		// check total time
// 		lua_pop(L, 1);
// 		return 0;
// 	}
// 	*start_time = lua_tonumber(L, -1);
// 	lua_pop(L,1);
// 	return 1;
// }

// static double get_time() {
// #if  !defined(__APPLE__)
// 	struct timespec ti;
// 	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ti);

// 	int sec = ti.tv_sec & 0xffff;
// 	int nsec = ti.tv_nsec;

// 	return (double)sec + (double)nsec / NANOSEC;
// #else
// 	struct task_thread_times_info aTaskInfo;
// 	mach_msg_type_number_t aTaskInfoCount = TASK_THREAD_TIMES_INFO_COUNT;
// 	if (KERN_SUCCESS != task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, (task_info_t )&aTaskInfo, &aTaskInfoCount)) {
// 		return 0;
// 	}

// 	int sec = aTaskInfo.user_time.seconds & 0xffff;
// 	int msec = aTaskInfo.user_time.microseconds;

// 	return (double)sec + (double)msec / MICROSEC;
// #endif
// }

// static int lstart(lua_State *L) {
// 	if (lua_gettop(L) != 0) {
// 		lua_settop(L,1);
// 		luaL_checktype(L, 1, LUA_TTHREAD);
// 	} else {
// 		lua_pushthread(L);
// 	}
// 	lua_Number start_time = 0;
// 	if (timing_enable(L, 1, &start_time)) {
// 		return luaL_error(L, "Thread %p start profile more than once", lua_topointer(L, 1));
// 	}

// 	// reset total time
// 	lua_pushvalue(L, 1);
// 	lua_pushnumber(L, 0);
// 	lua_rawset(L, lua_upvalueindex(2));

// 	// set start time
// 	lua_pushvalue(L, 1);
// 	start_time = get_time();
// #ifdef DEBUG_LOG
// 	fprintf(stderr, "PROFILE [%p] start\n", L);
// #endif
// 	lua_pushnumber(L, start_time);
// 	lua_rawset(L, lua_upvalueindex(1));

// 	return 0;
// }

// static int lstop(lua_State *L) {
// 	if (lua_gettop(L) != 0) {
// 		lua_settop(L,1);
// 		luaL_checktype(L, 1, LUA_TTHREAD);
// 	} else {
// 		lua_pushthread(L);
// 	}
// 	lua_Number start_time = 0;
// 	if (!timing_enable(L, 1, &start_time)) {
// 		return luaL_error(L, "Call profile.start() before profile.stop()");
// 	}
// 	double ti = diff_time(start_time);
// 	double total_time = timing_total(L,1);

// 	lua_pushvalue(L, 1);	// push coroutine
// 	lua_pushnil(L);
// 	lua_rawset(L, lua_upvalueindex(1));

// 	lua_pushvalue(L, 1);	// push coroutine
// 	lua_pushnil(L);
// 	lua_rawset(L, lua_upvalueindex(2));

// 	total_time += ti;
// 	lua_pushnumber(L, total_time);
// #ifdef DEBUG_LOG
// 	fprintf(stderr, "PROFILE [%p] stop (%lf/%lf)\n", lua_tothread(L,1), ti, total_time);
// #endif

// 	return 1;
// }

// static int init_profile(lua_State *L) {
// 	luaL_Reg l[] = {
// 		{ NULL, NULL },
// 	};
// 	luaL_newlibtable(L,l);
// 	lua_newtable(L);	// table thread->start time
// 	lua_newtable(L);	// table thread->total time

// 	lua_newtable(L);	// weak table
// 	lua_pushliteral(L, "kv");
// 	lua_setfield(L, -2, "__mode");

// 	lua_pushvalue(L, -1);
// 	lua_setmetatable(L, -3);
// 	lua_setmetatable(L, -3);

// 	luaL_setfuncs(L,l,2);

// 	return 1;
// }


static int cleardummy(lua_State *L) {
  return 0;
}

static int codecache(lua_State *L) {
	luaL_Reg l[] = {
		{ "clear", cleardummy },
		{ "mode", cleardummy },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	lua_getglobal(L, "loadfile");
	lua_setfield(L, -2, "loadfile");
	return 1;
}


OnbnetOnLua::OnbnetOnLua(): LuaState(nullptr), cbContext(nullptr) {

}

OnbnetOnLua::~OnbnetOnLua() {

}

// int OnbnetOnLua::operator()(int type, int session, uint32_t source, const void* msg, size_t sz) {
// 	lua_State *L = cbContext->L;
// 	int trace = 1;
// 	int r;
// 	lua_pushvalue(L,2);

// 	lua_pushinteger(L, type);
// 	lua_pushlightuserdata(L, const_cast<void*>(msg));
// 	lua_pushinteger(L,sz);
// 	lua_pushinteger(L, session);
// 	lua_pushinteger(L, source);

// 	r = lua_pcall(L, 5, 0 , trace);

// 	if (r == LUA_OK) {
// 		return 0;
// 	}

// 	lua_pop(L,1);
// 	return -1;
// }

int OnbnetOnLua::operator()(std::shared_ptr<Message>& msg) {
	lua_State *L = cbContext->L;
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

	lua_pop(L,1);
	return -1;
}

int OnbnetOnLua::Init(void* ctx, std::string fileName) { 
    lua_State *L = LuaState;
	lua_gc(L, LUA_GCSTOP, 0);
	lua_pushboolean(L, 1);  /* signal for libraries to ignore env. vars. */
	lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
	luaL_openlibs(L);
	openOnbnetLibs(L);
	// luaL_requiref(L, "onbnet.profile", init_profile, 0);

	// int profile_lib = lua_gettop(L);
	// // replace coroutine.resume / coroutine.wrap
	// lua_getglobal(L, "coroutine");
	// lua_getfield(L, profile_lib, "resume");
	// lua_setfield(L, -2, "resume");
	// lua_getfield(L, profile_lib, "wrap");
	// lua_setfield(L, -2, "wrap");

	// lua_settop(L, profile_lib-1);

	lua_pushlightuserdata(L, ctx);
	lua_setfield(L, LUA_REGISTRYINDEX, "OnbnetContext");
	luaL_requiref(L, "onbnet.codecache", codecache , 0);
	lua_pop(L,1);

	lua_gc(L, LUA_GCGEN, 0, 0);

	lua_pushstring(L, luaPath.c_str());
	lua_setglobal(L, "LUA_PATH");
	lua_pushstring(L, cPath.c_str());
	lua_setglobal(L, "LUA_CPATH");
	lua_pushstring(L, servicePath.c_str());
	lua_setglobal(L, "LUA_SERVICE");
	// const char *preload = skynet_command(ctx, "GETENV", "preload");
	// lua_pushstring(L, preload);
	// lua_setglobal(L, "LUA_PRELOAD");

	lua_pushcfunction(L, traceback);
	assert(lua_gettop(L) == 1);

	const char * loader = "./lualib/loader.lua";
	int r = luaL_loadfile(L,loader);
	if (r != LUA_OK) {
		return 1;
	}
	lua_pushlstring(L, fileName.c_str(), fileName.size());
	r = lua_pcall(L,1,0,1);
	if (r != LUA_OK) {
		printf("error: %s\n", lua_tostring(L, -1));
		return 1;
	}

	lua_settop(L,0);
	if (lua_getfield(L, LUA_REGISTRYINDEX, "memlimit") == LUA_TNUMBER) {

	}
	lua_pop(L, 1);

	lua_gc(L, LUA_GCRESTART, 0);
	return 0;
}

void OnbnetOnLua::CallBack() {
    luaL_checktype(LuaState,1,LUA_TFUNCTION);
	lua_settop(LuaState,1);
	cbContext = (CallbackContext*)lua_newuserdatauv(LuaState, sizeof(*cbContext), 2);
	cbContext->L = lua_newthread(LuaState);
	lua_pushcfunction(cbContext->L, traceback);
	lua_setiuservalue(LuaState, -2, 1);
	lua_getfield(LuaState, LUA_REGISTRYINDEX, "CallbackContext");
	lua_setiuservalue(LuaState, -2, 2);
	lua_setfield(LuaState, LUA_REGISTRYINDEX, "CallbackContext");
	lua_xmove(LuaState, cbContext->L, 1);
}

OnbnetOnLua* newOnbnetOnLua() {
    OnbnetOnLua* luaL = new OnbnetOnLua();
    lstate* ud = new lstate;
    ud->mem = 0;
    ud->mem_level = MEMLVL;
    luaL->LuaState = lua_newstate(lua_alloc, ud, 0);
    return luaL;
}