#define LUA_LIB

#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>

#include "libregexp.h"

#define CAPTURE_COUNT_MAX 255 /* from libregexp.c */

struct regex {
	uint8_t *bc;
};


static int regex_closure(lua_State *lstate)
{
	uint8_t *capture[CAPTURE_COUNT_MAX * 2];

	struct regex *r = lua_touserdata(lstate, lua_upvalueindex(1));
	const int global = lre_get_flags(r->bc) & LRE_FLAG_GLOBAL;
	const int capture_count = lre_get_capture_count(r->bc);

	const uint8_t *input = (uint8_t *) luaL_checkstring(lstate, 1);
	const int input_len = strlen((char *) input);

	lua_newtable(lstate);

	int nmatch = 0;

	int cindex = 0;
	while (lre_exec(capture, r->bc, input, cindex, input_len, 0, NULL) == 1) {

		// happens on a regex like (\\s*), don't know how we should handle it
		// and can't remember how vscode handles it
		if (capture[0] == capture[1]) {
			break;
		}

		nmatch++;
		cindex = capture[1] - input;

		lua_newtable(lstate);

		lua_pushnumber(lstate, 1 + capture[0] - input);
		lua_setfield(lstate, -2, "begin_ind");

		lua_pushnumber(lstate, 1 + capture[1] - input);
		lua_setfield(lstate, -2, "end_ind");

		lua_pushnumber(lstate, capture[1] - capture[0]);
		lua_setfield(lstate, -2, "length");

		lua_newtable(lstate);
		for (int i = 0; i < capture_count - 1; i++) {
			lua_pushlstring(lstate, (char *) capture[2 * i + 2], capture[2 * i + 3] - capture[2 * i + 2]);
			lua_rawseti(lstate, -2, i + 1);
		}
		lua_setfield(lstate, -2, "groups");

		lua_rawseti(lstate, -2, nmatch);

		if (!global) {
			break;
		}
	}

	return 1;
}


static int jsregexp_gc(lua_State *lstate)
{
	struct regex *r = lua_touserdata(lstate, 1);
	free(r->bc);
	return 0;
}


/* static int jsregexp_tostring(lua_State *lstate) */
/* { */
/* 	lua_pushfstring(lstate, "jsregexp: %p", lua_touserdata(lstate, 1)); */
/* 	return 1; */
/* } */


static struct luaL_Reg jsregexp_meta[] = {
  {"__gc", jsregexp_gc},
  /* {"__tostring", jsregexp_tostring}, */
  {NULL, NULL}
};


static int jsregexp_compile(lua_State *lstate)
{
	char error_msg[64];
	int len, re_flags = 0;

	const char *regex = luaL_checkstring(lstate, 1);

	if (!lua_isnoneornil(lstate, 2)) {
		const char *flags = luaL_checkstring(lstate, 2);
		while (*flags) {
			switch (*(flags++)) {
				case 'i': re_flags |= LRE_FLAG_IGNORECASE; break;
				case 'g': re_flags |= LRE_FLAG_GLOBAL; break;
				default: /* unknown flag */;
			}
		}
	}

	uint8_t *bc = lre_compile(&len, error_msg, sizeof error_msg, regex,
			strlen(regex), re_flags, NULL);
	if (!bc) {
		lua_pushnil(lstate);
		lua_pushstring(lstate, error_msg);
		return 2;
	}

	struct regex *ud = lua_newuserdata(lstate, sizeof *ud);
	ud->bc = bc;

	if (luaL_newmetatable(lstate, "jsregexp_meta")) {
		luaL_register(lstate, NULL, jsregexp_meta);
	}
	lua_setmetatable(lstate, -2);

	lua_pushcclosure(lstate, regex_closure, 1);
	return 1;
}


static const struct luaL_Reg jsregexp_lib[] = {
	{"compile", jsregexp_compile},
	{NULL, NULL}
};


int luaopen_jsregexp(lua_State *lstate)
{
	luaL_openlib(lstate, "jsregexp", jsregexp_lib, 0);
	return 1;
}
