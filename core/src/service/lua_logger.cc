
#include "logger.h"
#include "lua_logger.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

int llog_info(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_info(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lalog_info(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_info(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int llog_error(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_error(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lalog_error(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_error(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int llog_debug(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_debug(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lalog_debug(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_debug(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int llog_warn(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        log_warn(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lalog_warn(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        alog_warn(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lconsole_info(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_info(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lconsole_error(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_error(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lconsole_debug(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_debug(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int lconsole_warn(lua_State *L) {
    size_t len = 0;
    auto text = luaL_checklstring(L, 1, &len);
    if (len > 0) {
        console_warn(text);
    } else {
        luaL_error(L, "log_info param error");
    }
    return 0;
}

int luaopen_onbnet_loggerdriver(lua_State *L) {
	luaL_checkversion(L);
    
	luaL_Reg l[] = {
        {"log_info", llog_info},
        {"alog_info", lalog_info},
        {"log_error", llog_error},
        {"alog_error", lalog_error},
        {"log_debug", llog_debug},
        {"alog_debug", lalog_debug},
        {"log_debug", llog_debug},
        {"alog_debug", lalog_debug},
        {"log_warn", llog_warn},
        {"alog_warn", lalog_warn},
        {"console_info", lconsole_info},
        {"console_debug", lconsole_debug},
        {"console_error", lconsole_error},
        {"console_warn", lconsole_warn},
		{ NULL, NULL },
	};

    luaL_newlib(L, l);

	return 1;
}