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

    if (capture[0] == capture[1]) {
      // empty match -> continue matching from next character (to prevent an endless loop).
      // This is basically the same implementation as in quickjs, see
      // https://github.com/bellard/quickjs/blob/2788d71e823b522b178db3b3660ce93689534e6d/quickjs.c#L42857-L42869

      // +1 works for ascii, take a closer look for unicode
      ++cindex;
    } else {
      cindex = capture[1] - input;
    }

    lua_newtable(lstate);

    lua_pushnumber(lstate, 1 + capture[0] - input);
    lua_setfield(lstate, -2, "begin_ind");

    lua_pushnumber(lstate, 1 + capture[1] - input);
    lua_setfield(lstate, -2, "end_ind");

    lua_pushnumber(lstate, capture[1] - capture[0]);
    lua_setfield(lstate, -2, "length");

    lua_newtable(lstate);
    for (int i = 1; i < capture_count; i++) {
      lua_pushlstring(lstate, (char *) capture[2 * i], capture[2 * i + 1] - capture[2 * i]);
      lua_rawseti(lstate, -2, i);
    }
    lua_setfield(lstate, -2, "groups");

    lua_rawseti(lstate, -2, ++nmatch);

    if (!global || cindex > input_len) {
      break;
    }
  }

  return 1;
}


static int regexp_gc(lua_State *lstate)
{
  struct regex *r = lua_touserdata(lstate, 1);
  free(r->bc);
  return 0;
}


/* static int regexp_tostring(lua_State *lstate) */
/* { */
/*  lua_pushfstring(lstate, "jsregexp: %p", lua_touserdata(lstate, 1)); */
/*  return 1; */
/* } */


static struct luaL_Reg jsregexp_meta[] = {
  {"__gc", regexp_gc},
  /* {"__tostring", regexp_tostring}, */
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
