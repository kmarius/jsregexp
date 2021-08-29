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

static int match_closure(lua_State *L)
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

	str_builder_clear(sb);
	for (i = 1, cindex = 0;
			lre_exec(capture, r->bc, input, cindex, input_len, 0, NULL) == 1;
			i++) {

		// capture[0] - begin of complete match
		// capture[1] - end of complete match

		if (capture[0] != input + cindex) {
			str_builder_add_str(sb, (char*)input+cindex, capture[0]-input-cindex);
		}

		format_apply(r->fmt, sb, (char**)capture, capture_count);

		/* TODO: infinite loop when (\\w*) matches with nothing? (on 2021-08-28) */
		cindex = capture[1] - input;
		if (!global) {
			break;
		}
	}
	str_builder_add_str(sb, (char*)input+cindex, 0);
	lua_pushstring(L, str_builder_peek(sb));
	str_builder_clear(sb);
	return 1;
}

static int regex_gc(lua_State *L)
{
	struct replacer_t *r;
	r = lua_touserdata(L, 1);
	free(r->bc);
	format_destroy(r->fmt);
	return 0;
}

static int jsregexp_compile(lua_State *L)
{
	uint8_t *bc;
	struct replacer_t *ud;
	int len, re_flags = 0;
	char error_msg[64];
	const char *regex;
	format_t *fmt;

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
	if (NULL == (fmt = format_create(luaL_checkstring(L, 3), error_msg, sizeof(error_msg)))) {
		free(bc);
		lua_pushnil(L);
		lua_pushstring(L, error_msg);
		return 2;
	}

	ud = lua_newuserdata(L, sizeof(*ud));
	ud->bc = bc;
	ud->fmt = fmt;

	if (luaL_newmetatable(L, "jsregexp_bc")) {
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
	/* setlocale(LC_ALL, "de_DE.UTF-8"); */
	setlocale(LC_ALL, "en_US.UTF-8");
	luaL_openlib(L, "jsregexp", lib, 0);
	sb = str_builder_create();
	return 1;
}
