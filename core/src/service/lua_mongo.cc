
#include "lua_mongo.h"

#include <cstdint>
#include <cstring>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "jemalloc.h"
}

#define OP_COMPRESSED 2012
#define OP_MSG 2013

typedef enum {
	MSG_CHECKSUM_PRESENT = 1 << 0,
	MSG_MORE_TO_COME = 1 << 1,
	MSG_EXHAUST_ALLOWED = 1 << 16,
} msg_flags_t;

struct header_t {
	//int32_t message_length; 	// total message size, include this
	int32_t request_id;			// identifier for this message
	int32_t response_to;		// requestID from the original request(used in responses from the database)
	int32_t opcode;			// message type

	int32_t flags;
};

#define DEFAULT_CAP 128
struct buffer {
	int size;
	int cap;
	uint8_t * ptr;
	uint8_t buffer[DEFAULT_CAP];
};

typedef void * document;

static inline uint32_t little_endian(uint32_t v) {
	union {
		uint32_t v;
		uint8_t b[4];
	} u;
	u.v = v;
	return u.b[0] | u.b[1] << 8 | u.b[2] << 16 | u.b[3] << 24;
}

static inline uint32_t get_length(document buffer) {
	union {
		uint32_t v;
		uint8_t b[4];
	} u;
	memcpy(&u.v, buffer, 4);
	return u.b[0] | u.b[1] << 8 | u.b[2] << 16 | u.b[3] << 24;
}

static inline void buffer_destroy(struct buffer *b) {
	if (b->ptr != b->buffer) {
		je_free(b->ptr);
	}
}

static inline void buffer_create(struct buffer *b) {
	b->size = 0;
	b->cap = DEFAULT_CAP;
	b->ptr = b->buffer;
}

static inline void buffer_reserve(struct buffer *b, int sz) {
	if (b->size + sz <= b->cap)
		return;
	do {
		b->cap *= 2;
	} while (b->cap <= b->size + sz);

	if (b->ptr == b->buffer) {
		b->ptr = (uint8_t*)je_malloc(b->cap);
		memcpy(b->ptr, b->buffer, b->size);
	} else {
		b->ptr = (uint8_t*)je_realloc(b->ptr, b->cap);
	}
}

static inline void write_int32(struct buffer *b, int32_t v) {
	uint32_t uv = (uint32_t)v;
	buffer_reserve(b,4);
	b->ptr[b->size++] = uv & 0xff;
	b->ptr[b->size++] = (uv >> 8)&0xff;
	b->ptr[b->size++] = (uv >> 16)&0xff;
	b->ptr[b->size++] = (uv >> 24)&0xff;
}

static inline void write_int8(struct buffer *b, int8_t v) {
	uint8_t uv = (uint8_t)v;
	buffer_reserve(b, 1);
	b->ptr[b->size++] = uv;
}

static inline int reserve_length(struct buffer *b) {
	int sz = b->size;
	buffer_reserve(b,4);
	b->size +=4;
	return sz;
}

static inline void write_length(struct buffer *b, int32_t v, int off) {
	uint32_t uv = (uint32_t)v;
	b->ptr[off++] = uv & 0xff;
	b->ptr[off++] = (uv >> 8)&0xff;
	b->ptr[off++] = (uv >> 16)&0xff;
	b->ptr[off++] = (uv >> 24)&0xff;
}

static int unpack_reply(lua_State *L) {
	size_t data_len = 0;
	const char * data = luaL_checklstring(L,1,&data_len);
	const struct header_t* h = (const struct header_t*)data;

	if (data_len < sizeof(struct header_t)) {
		lua_pushboolean(L, 0);
		return 1;
	}

	int opcode = little_endian(h->opcode);
	if (opcode != OP_MSG) {
		return luaL_error(L, "Unsupported opcode:%d", opcode);
	}

	int id = little_endian(h->response_to);
	int flags = little_endian(h->flags);

	if (flags != 0) {
		if ((flags & MSG_CHECKSUM_PRESENT) != 0)  {
			return luaL_error(L, "Unsupported OP_MSG flag checksumPresent");
		}

		if ((flags ^ MSG_MORE_TO_COME) != 0) {
			return luaL_error(L, "Unsupported OP_MSG flag:%d", flags);
		}
	}

	int sz = (int)data_len - sizeof(*h);

	const uint8_t * section = (const uint8_t *)(h+1);

	uint8_t payload_type = *section;
	const uint8_t * doc = section+1;

	if (payload_type != 0) {
		return luaL_error(L, "Unsupported OP_MSG payload type: %d", payload_type);
	}

	int32_t doc_sz = get_length((document)(doc));
	if ((sz - 1) != doc_sz) {
		return luaL_error(L, "Unsupported OP_MSG reply: >1 section");
	}

	lua_pushboolean(L, 1);
	lua_pushinteger(L, id);
	lua_pushlightuserdata(L, (void *)(doc));
	return 3;
}

static int reply_length(lua_State *L) {
	const char * rawlen_str = luaL_checkstring(L, 1);
	int rawlen = 0;
	memcpy(&rawlen, rawlen_str, sizeof(int));
	int length = little_endian(rawlen);
	lua_pushinteger(L, length - 4);
	return 1;
}

static int op_msg(lua_State *L) {
	int id = luaL_checkinteger(L, 1);
	int flags = luaL_checkinteger(L, 2);
	document cmd = lua_touserdata(L, 3);

	if (cmd == NULL) {
		return luaL_error(L, "opmsg require cmd document");
	}

	luaL_Buffer b;
	luaL_buffinit(L, &b);

	struct buffer buf;
	buffer_create(&buf);
	int len = reserve_length(&buf);
	write_int32(&buf, id);
	write_int32(&buf, 0);
	write_int32(&buf, OP_MSG);
	write_int32(&buf, flags);
	write_int8(&buf, 0);

	int32_t cmd_len = get_length(cmd);
	int total = buf.size + cmd_len;

	write_length(&buf, total, len);
	luaL_addlstring(&b, (const char *)buf.ptr, buf.size);
	buffer_destroy(&buf);

	luaL_addlstring(&b, (const char *)cmd, cmd_len);
	luaL_pushresult(&b);
	return 1;
}

int luaopen_onbnet_mongo_driver(lua_State *L) {
    luaL_checkversion(L);
	luaL_Reg l[] ={
		{ "reply", unpack_reply }, // 接收响应
		{ "length", reply_length },
		{ "op_msg", op_msg},
		{ NULL, NULL },
	};

	luaL_newlib(L,l);
	return 1;
}
