#define LUA_LIB

#include <lauxlib.h>
#include <locale.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

#include "format.h"
#include "libregexp.h"
#include "str_builder.h"

#define CAPTURE_COUNT_MAX 255 /* from libregexp.c */

struct replacer {
	uint8_t *bc;
	Trafo *trafo;
};

static str_builder_t *sb;


static int l_trafo_closure(lua_State *L)
{
	uint8_t *capture[CAPTURE_COUNT_MAX * 2];

	struct replacer *r = lua_touserdata(L, lua_upvalueindex(1));
	const int global = lre_get_flags(r->bc) & LRE_FLAG_GLOBAL;
	const int capture_count = lre_get_capture_count(r->bc);

	const uint8_t *input = (uint8_t *) luaL_checkstring(L, 1);
	const int input_len = strlen((char *) input);

	str_builder_clear(sb);

	uint16_t nmatch = 0;
	int cindex = 0;
	while (lre_exec(capture, r->bc, input, cindex, input_len, 0, NULL) == 1) {
		nmatch++;

		if (capture[0] != input + cindex) {
			str_builder_add_str(sb, (char *) input+cindex,
					capture[0]-input-cindex);
		}

		trafo_apply(r->trafo, sb, (char **) capture, capture_count);

		cindex = capture[1] - input;

		// Infinite loops can happen when (\\s*) matches with nothing e.g. on
		// a bad regex like (\\s*)|(\\w+) I checked an online js repl and
		// I think it just aborts on a match of length 0 ? vscode does one
		// transform (i.e. inserts the format since nothing is removed).
		if (capture[0] == capture[1]) {
			break;
		}

		if (!global) {
			break;
		}
	}
	if (nmatch == 0 && trafo_has_else(r->trafo)) {
		// Trigger 'else' part of ${N:?*:*} and ${N:-*} placeholders
		trafo_apply(r->trafo, sb, NULL, 0);
	} else {
		str_builder_add_str(sb, (char *) input + cindex, 0);
	}
	lua_pushstring(L, str_builder_peek(sb));
	str_builder_clear(sb);
	return 1;
}


static int l_transform_gc(lua_State *L)
{
	struct replacer *r = lua_touserdata(L, 1);
	free(r->bc);
	trafo_destroy(r->trafo);
	return 0;
}


static int l_jsregexp_transformer(lua_State *L)
{
	char error_msg[64];
	int len, re_flags = 0;

	const char *regex = luaL_checkstring(L, 1);

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

	uint8_t *bc = lre_compile(&len, error_msg, sizeof(error_msg), regex,
			strlen(regex), re_flags, NULL);
	if (!bc) {
		lua_pushnil(L);
		lua_pushstring(L, error_msg);
		return 2;
	}

	Trafo *trafo = trafo_create(luaL_checkstring(L, 2), error_msg,
			sizeof(error_msg));
	if (!trafo) {
		free(bc);
		lua_pushnil(L);
		lua_pushstring(L, error_msg);
		return 2;
	}

	struct replacer *ud = lua_newuserdata(L, sizeof(*ud));
	ud->bc = bc;
	ud->trafo = trafo;

	if (luaL_newmetatable(L, "jsregexp_transform")) {
		lua_pushcfunction(L, l_transform_gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);

	lua_pushcclosure(L, l_trafo_closure, 1);
	return 1;
}


static const struct luaL_Reg lib[] = {
	{"transformer", l_jsregexp_transformer},
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
