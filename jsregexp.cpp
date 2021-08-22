#define LUA_LIB

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifdef __cplusplus
}
#endif

#include <regex>
#include <string>

static int jsregexp_match(lua_State *L) {
	lua_newtable(L);
	if (!lua_isstring(L, 1)) {
		// print error
		return 1;
	}
	if (!lua_isstring(L, 2)) {
		// print error
		// lua_pushvalue(L, 1); // return input unchanged
		return 1;
	}
	std::regex regex;
	try {
		regex = std::regex(lua_tostring(L, 2));
	} catch (const std::regex_error& e) {
		// print error
		// lua_pushvalue(L, 1); // return input unchanged
		return 1;
	}

	std::string s(lua_tostring(L, 1));
	std::smatch m;

	for (int i = 1, pos = 1; std::regex_search(s, m, regex); i++) {
		lua_newtable(L);
		for (int j = 0; j < m.size(); j++) {
			lua_pushstring(L, m[j].str().c_str());
			lua_rawseti(L, -2, j);
		}
		lua_pushinteger(L, m.position() + pos);
		lua_setfield(L, -2, "position");
		lua_pushinteger(L, m.length());
		lua_setfield(L, -2, "length");
		lua_rawseti(L, -2, i);
		pos += m.length();
		s = m.suffix().str();
	}

	return 1;
}

static const struct luaL_Reg testlib[] = {
	{"gmatch", jsregexp_match},
	{NULL, NULL}
};

extern "C"
int luaopen_jsregexp(lua_State *L) {
	luaL_openlib(L, "jsregexp", testlib, 0);
	return 1;
}
