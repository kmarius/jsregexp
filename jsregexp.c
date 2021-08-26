#define LUA_LIB

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>

#include "libregexp.h"

#define CAPTURE_COUNT_MAX 255 /* from libregexp.c */

static int match_closure(lua_State *L)
{
	uint8_t *bc, *capture[CAPTURE_COUNT_MAX * 2];
	const uint8_t *input;
	char *buf;
	int capture_count, global, input_len, cindex, i, j;

	bc = *(uint8_t**)lua_touserdata(L, lua_upvalueindex(1));
	global = lre_get_flags(bc) & LRE_FLAG_GLOBAL;
	capture_count = lre_get_capture_count(bc);

	input = (uint8_t*)luaL_checkstring(L, 1);
	input_len = strlen((char*)input);

	buf = malloc(sizeof(*buf) * (input_len + 1));
	// check error

	lua_newtable(L);
	for (i = 1, cindex = 0;
			lre_exec(capture, bc, input, cindex, input_len, 0, NULL) == 1;
			cindex = capture[1] - input,
			i++) {

		lua_newtable(L);

		for (j = 0; j < capture_count; j++) {
			uint8_t *begin = capture[2*j];
			uint8_t *end = capture[2*j+1];
			if (!begin) {
				// push nil instead?
				lua_pushstring(L, "");
			} else {
				memcpy(buf, begin, end-begin);
				buf[end-begin] = 0;
				lua_pushstring(L, buf);
				lua_rawseti(L, -2, j);
			}
		}

		// These need to be recalculated for multibyte strings
		lua_pushinteger(L, capture[0]-input);
		lua_setfield(L, -2, "position");

		lua_pushinteger(L, capture[1]-capture[0]);
		lua_setfield(L, -2, "length");

		lua_rawseti(L, -2, i);

		if (!global) {
			break;
		}
	}

	free(buf);

	return 1;
}

static int regex_gc(lua_State *L)
{
	free(*(char**)lua_touserdata(L, 1));
	return 0;
}

static int jsregexp_compile(lua_State *L)
{
	uint8_t *bc, **ud;
	int len, i, re_flags = 0;
	char error_msg[64];
	const char *regex;

	regex = luaL_checkstring(L, 1);

	if (!lua_isnoneornil(L, 2)) {
		const char *flags = luaL_checkstring(L, 2);
		while (*flags) {
			switch (*(flags++)) {
				case 'i': re_flags |= LRE_FLAG_IGNORECASE; break;
				case 'g': re_flags |= LRE_FLAG_GLOBAL; break;
				default: /* unknown flag */;
			}
		}
	}

	if (NULL == (bc = lre_compile(&len, error_msg, sizeof(error_msg),
					regex, strlen(regex), re_flags, NULL))) {
		lua_pushnil(L);
		lua_pushstring(L, error_msg);
		return 2;
	}

	ud = (uint8_t**) lua_newuserdata(L, sizeof(*ud));
	*ud = bc;

	if (luaL_newmetatable(L, "regmeta")) {
		lua_pushcfunction(L, regex_gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);

	lua_pushcclosure(L, match_closure, 1);
	return 1;
}

static const struct luaL_Reg lib[] = {
	{"compile", jsregexp_compile},
	{NULL, NULL}
};

int luaopen_jsregexp(lua_State *L)
{
	luaL_openlib(L, "jsregexp", lib, 0);
	return 1;
}
