#define LUA_LIB

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include "libregexp.h"
#include "str_builder.h"
#include "format.h"

#define CAPTURE_COUNT_MAX 255 /* from libregexp.c */

struct replacer_t {
	uint8_t *bc;
	format_t *fmt;
};

static str_builder_t *sb;

static int transform_closure(lua_State *L)
{
	uint8_t *capture[CAPTURE_COUNT_MAX * 2];
	const uint8_t *input;
	char *buf;
	int capture_count, global, input_len, cindex, i, j;
	struct replacer_t *r;

	r = lua_touserdata(L, lua_upvalueindex(1));
	global = lre_get_flags(r->bc) & LRE_FLAG_GLOBAL;
	capture_count = lre_get_capture_count(r->bc);

	input = (uint8_t*)luaL_checkstring(L, 1);
	input_len = strlen((char*)input);

	int nmatch = 0;
	str_builder_clear(sb);
	for (i = 1, cindex = 0;
			lre_exec(capture, r->bc, input, cindex, input_len, 0, NULL) == 1;
			i++) {
		nmatch++;

		if (capture[0] != input + cindex) {
			str_builder_add_str(sb, (char*)input+cindex, capture[0]-input-cindex);
		}

		format_apply(r->fmt, sb, (char**)capture, capture_count);

		/* Infinite loops can happen when (\\s*) matches with nothing
		 * e.g. on a bad regex like (\\s*)|(\\w+)
		 * I checked an online js repl and I think it just aborts on a match of length 0 ?
		 * vscode does one transform (i.e. inserts the format since nothing is removed).
		 * */
		if (capture[0] == capture[1]) {
			break;
		}

		cindex = capture[1] - input;
		if (!global) {
			break;
		}
	}
	if (nmatch == 0) {
		// Trigger 'else' part of ${N:?*:*} and ${N:-*} placeholders
		format_apply(r->fmt, sb, NULL, 0);
	} else {
		str_builder_add_str(sb, (char*)input+cindex, 0);
	}
	lua_pushstring(L, str_builder_peek(sb));
	str_builder_clear(sb);
	return 1;
}

static int transform_gc(lua_State *L)
{
	struct replacer_t *r;
	r = lua_touserdata(L, 1);
	free(r->bc);
	format_destroy(r->fmt);
	return 0;
}

static int jsregexp_transform(lua_State *L)
{
	uint8_t *bc;
	struct replacer_t *ud;
	int len, re_flags = 0;
	char error_msg[64];
	const char *regex;
	format_t *fmt;

	regex = luaL_checkstring(L, 1);

	if (!lua_isnoneornil(L, 3)) {
		const char *flags = luaL_checkstring(L, 3);
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
	if (NULL == (fmt = format_create(luaL_checkstring(L, 2), error_msg, sizeof(error_msg)))) {
		free(bc);
		lua_pushnil(L);
		lua_pushstring(L, error_msg);
		return 2;
	}

	ud = lua_newuserdata(L, sizeof(*ud));
	ud->bc = bc;
	ud->fmt = fmt;

	if (luaL_newmetatable(L, "jsregexp_transform")) {
		lua_pushcfunction(L, transform_gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);

	lua_pushcclosure(L, transform_closure, 1);
	return 1;
}

static const struct luaL_Reg lib[] = {
	{"transformer", jsregexp_transform},
	{NULL, NULL}
};

int luaopen_jsregexp(lua_State *L)
{
	sb = str_builder_create();
	/* setlocale(LC_ALL, "de_DE.UTF-8"); */
	setlocale(LC_ALL, "en_US.UTF-8");
	luaL_openlib(L, "jsregexp", lib, 0);
	return 1;
}
