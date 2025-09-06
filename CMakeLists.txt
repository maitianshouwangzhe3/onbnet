# CMake 最低版本要求
cmake_minimum_required(VERSION 3.14)

# 项目名称
project(onbnet)

set(CMAKE_CXX_STANDARD 17)


# 设置默认构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# 设置 LuaJIT 路径
set(LUAJIT_DIR ${PROJECT_SOURCE_DIR}/3rd/LuaJIT)
set(LUAJIT_INC ${LUAJIT_DIR}/src)
set(LUAJIT_LIBPATH ${LUAJIT_INC})

set(LUA_DIR ${PROJECT_SOURCE_DIR}/3rd/lua)
set(LUA_INC ${LUA_DIR})
set(LUA_LIBPATH ${LUA_INC})

# Jemalloc 路径
set(JEMALLOC_DIR ${PROJECT_SOURCE_DIR}/3rd/jemalloc)
set(JEMALLOC_INC ${JEMALLOC_DIR}/include/jemalloc)

# Luna 路径
set(LUNA_DIR ${PROJECT_SOURCE_DIR}/3rd/luna)
set(LUNA_INC ${LUNA_DIR})
set(LUNA_LIBPATH ${LUNA_DIR})

set(ONBNET_INC ${PROJECT_SOURCE_DIR}/core/include)
set(ONBNET_BASE_INC ${PROJECT_SOURCE_DIR}/core/include/Base)
set(ONBNET_NET_INC ${PROJECT_SOURCE_DIR}/core/include/Net)
set(ONBNET_UTIL_INC ${PROJECT_SOURCE_DIR}/core/include/Util)
set(ONBNET_POLLER_INC ${PROJECT_SOURCE_DIR}/core/include/Poller)
set(ONBNET_BUFFER_INC ${PROJECT_SOURCE_DIR}/core/include/Buffer)
set(ONBNET_THREAD_INC ${PROJECT_SOURCE_DIR}/core/include/Worker)
set(ONBNET_SERVICE_INC ${PROJECT_SOURCE_DIR}/core/include/Service)
set(ONBNET_MESSAGE_INC ${PROJECT_SOURCE_DIR}/core/include/Message)

# 核心源码路径
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src ONBNET_SRC_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Net ONBNET_SRC_NET_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Util ONBNET_SRC_UTIL_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Base ONBNET_SRC_BASE_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Poller ONBNET_SRC_POLLER_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Buffer ONBNET_SRC_BUFFER_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Worker ONBNET_SRC_THREAD_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Service ONBNET_SRC_SERVICE_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/core/src/Message ONBNET_SRC_MESSAGE_FILES)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/luaclib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)


# 编译选项
add_compile_options(-g -O0 -Wall -DLUAJIT_ENABLE_LUA52COMPAT)
add_compile_options(-fno-omit-frame-pointer)

# 条件编译定义
set(SHARED_LIB_FLAGS -fPIC --shared)
set(EXPORT_FLAG -Wl,-E)
add_compile_options(-fPIC)
add_link_options(${EXPORT_FLAG})
set(ONBNET_LIBS -ldl -lm -lpthread -lcrypto)

# 添加子模块构建
include(ExternalProject)
ExternalProject_Add(jemalloc_ext
    PREFIX ${PROJECT_SOURCE_DIR}/3rd/jemalloc
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rd/jemalloc
    CONFIGURE_COMMAND <SOURCE_DIR>/autogen.sh && <SOURCE_DIR>/configure 
        --prefix=<INSTALL_DIR> 
        --with-jemalloc-prefix=je_ 
        --enable-cc-silence
    BUILD_COMMAND make -j1
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
)

# # 构建 LuaJIT
# ExternalProject_Add(luajit2_ext
#     PREFIX ${PROJECT_SOURCE_DIR}/3rd/luajit2
#     SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rd/luajit2
#     CONFIGURE_COMMAND ""
#     BUILD_COMMAND make -j1
#     INSTALL_COMMAND ""
#     BUILD_IN_SOURCE 1
# ) 

# 构建 Lua
ExternalProject_Add(lua_ext
    PREFIX ${PROJECT_SOURCE_DIR}/3rd/lua
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rd/lua
    PATCH_COMMAND ${CMAKE_COMMAND} -E echo "Adding -fPIC to CFLAGS"
    COMMAND sed -i "s/CFLAGS= -Wall -O2/CFLAGS= -Wall -O2 -fPIC/" makefile
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -j1
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
)

# 构建 Luajit
ExternalProject_Add(luajit_ext
    PREFIX ${PROJECT_SOURCE_DIR}/3rd/LuaJIT
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rd/LuaJIT
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -j1
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
)

# 构建 Luna
ExternalProject_Add(luna_ext
    PREFIX ${PROJECT_SOURCE_DIR}/3rd/luna
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rd/luna
    PATCH_COMMAND ${CMAKE_COMMAND} -E echo "Adding -fPIC to CFLAGS"
    COMMAND sed -i "s/CFLAGS =/CFLAGS= -fPIC/" makefile
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -j1
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
)

# 构建
# ExternalProject_Add(lua-serialize
#     PREFIX ${PROJECT_SOURCE_DIR}/3rd/lua-serialize
#     SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rd/lua-serialize
#     CONFIGURE_COMMAND ""
#     BUILD_COMMAND make linux
#     INSTALL_COMMAND ""
#     BUILD_IN_SOURCE 1
# )

# 主程序 onbnet
set(ONBNET_SOURCES
    ${ONBNET_SRC_FILES}
    ${ONBNET_SRC_NET_FILES}
    ${ONBNET_SRC_UTIL_FILES}
    ${ONBNET_SRC_BASE_FILES}
    ${ONBNET_SRC_POLLER_FILES}
    ${ONBNET_SRC_BUFFER_FILES}
    ${ONBNET_SRC_THREAD_FILES}
    ${ONBNET_SRC_SERVICE_FILES}
    ${ONBNET_SRC_MESSAGE_FILES}
)

# set(ONBNET_DY_SOURCES
#     ${ONBNET_SRC_SERVICE_FILES}
#     ${ONBNET_SRC_MESSAGE_FILES}
# )

# set(ONBNET_DY_INCLUDES
#     ${ONBNET_BASE_INC}
#     ${ONBNET_SERVICE_INC}
# )

# 头文件目录
set(ONBNET_INCLUDES
    ${LUA_INC}
    ${LUNA_INC}
    ${JEMALLOC_INC}
    ${ONBNET_INC}
    ${ONBNET_UTIL_INC}
    ${ONBNET_POLLER_INC}
    ${ONBNET_BASE_INC}
    ${ONBNET_NET_INC}
    ${ONBNET_BUFFER_INC}
    ${ONBNET_THREAD_INC}
    ${ONBNET_SERVICE_INC}
    ${ONBNET_MESSAGE_INC}
)

# 构建 luaclib/onlua.so
# set(LUA_CLIB_ONLUA_SRC
#     ${PROJECT_SOURCE_DIR}/core/src/Service/OnbnetOnLua.cc
# )

# set(LUA_CLIB_ONLUA_INC
#     ${LUNA_INC}
#     ${LUA_INC}
#     ${JEMALLOC_INC}
#     ${ONBNET_BASE_INC}
#     ${ONBNET_SERVICE_INC}
# )
# #${LUA_LIBPATH}/liblua.a
# add_library(onlua SHARED ${LUA_CLIB_ONLUA_SRC})
# add_dependencies(onlua lua_ext luna_ext)
# target_include_directories(onlua PRIVATE ${LUA_CLIB_ONLUA_INC})
# target_link_libraries(onlua PRIVATE ${JEMALLOC_DIR}/lib/libjemalloc_pic.a ${LUA_LIBPATH}/liblua.a)
# target_link_options(onlua PRIVATE ${SHARED_LIB_FLAGS})
# set_target_properties(onlua PROPERTIES PREFIX "" OUTPUT_NAME "onlua" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# 构建 luaclib/lua_onbnet.so
# set(LUA_CLIB_ONBNET_SRC
#     # ${ONBNET_SRC_SERVICE_FILES}
#     ${ONBNET_SRC_UTIL_FILES}
#     ${ONBNET_SRC_MESSAGE_FILES}
#     ${PROJECT_SOURCE_DIR}/core/src/Service/Service.cc
#     ${PROJECT_SOURCE_DIR}/core/src/Service/luaOnbnet.cc
#     ${PROJECT_SOURCE_DIR}/core/src/Service/ServiceManager.cc
# )

# set(LUA_CLIB_ONBNET_INC
#     ${LUA_INC}
#     ${LUNA_INC}
#     ${ONBNET_INC}
#     ${ONBNET_BASE_INC}
#     ${ONBNET_UTIL_INC}
#     ${ONBNET_SERVICE_INC}
#     ${ONBNET_MESSAGE_INC}
# )

# add_library(lua_onbnet SHARED ${LUA_CLIB_ONBNET_SRC})
# add_dependencies(lua_onbnet lua_ext luna_ext)
# target_include_directories(lua_onbnet PRIVATE ${LUA_CLIB_ONBNET_INC})
# target_link_libraries(lua_onbnet PRIVATE ${LUA_LIBPATH}/liblua.a ${LUNA_LIBPATH}/libluna.a)
# target_link_options(lua_onbnet PRIVATE ${SHARED_LIB_FLAGS})
# set_target_properties(lua_onbnet PROPERTIES PREFIX "" OUTPUT_NAME "onbnetcore" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# 构建 onbnet
add_executable(onbnet ${ONBNET_SOURCES})
add_dependencies(onbnet lua_ext jemalloc_ext luna_ext)
target_include_directories(onbnet PRIVATE ${ONBNET_INCLUDES})
target_link_options(onbnet PRIVATE ${ONBNET_LIBS})
target_link_libraries(onbnet PRIVATE ${JEMALLOC_DIR}/lib/libjemalloc_pic.a ${LUA_LIBPATH}/liblua.a ${LUNA_LIBPATH}/libluna.a)


# 构建 luaclib/ltls.so (如果需要)
# 注意：ltls 需要链接 OpenSSL，这里假设已安装
# add_library(lua_ltls SHARED lualib-src/ltls.cc)
# target_include_directories(lua_ltls PRIVATE ${LUA_INC})
# target_link_libraries(lua_ltls PRIVATE ssl crypto)
# target_link_options(lua_ltls PRIVATE ${SHARED_LIB_FLAGS})
# set_target_properties(lua_ltls PROPERTIES PREFIX "" OUTPUT_NAME "ltls" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# 安装目标（可选）
install(TARGETS onbnet DESTINATION .)