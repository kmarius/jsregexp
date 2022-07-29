#define LUA_LIB

#include <lauxlib.h>
#include <lua.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cutils.h"
#include "libregexp.h"

#define CAPTURE_COUNT_MAX 255 /* from libregexp.c */

#if LUA_VERSION_NUM >= 502
#define new_lib(L, l) (luaL_newlib(L, l))
#define lua_tbl_len(L, arg) (lua_rawlen(L, arg))
#else
#define new_lib(L, l) (lua_newtable(L), luaL_register(L, NULL, l))
#define lua_tbl_len(L, arg) (lua_objlen(L, arg))
#endif

#if LUA_VERSION_NUM < 503
#define lua_pushinteger(L, n) lua_pushinteger(L, n)
#endif

struct regex {
  uint8_t *bc;
};


// libregex expects the regex encoded as CESU8: https://en.wikipedia.org/wiki/CESU-8
// (utf16 didn't work)

// check for bytes larger or equal to 0xF0, the corresponding code point gets encoded
// with 3 bytes in CESU8
static inline bool utf8_contains_cesu8(const char *s)
{
  uint8_t *q = (uint8_t *) s;
  while (*q) {
    if ((*q++ & 0xf0) == 0xf0) {
      return true;
    }
  }
  return false;
}


// TODO: we assume valid utf8 here, think about at least checking if the string
// doesn't end and we access unknown memory
static inline char *utf8_to_cesu8(const char *src, int *l)
{
  char *str = malloc(3 * strlen(src) / 2 + 1);
  char *q = str;

  while (*src) {
    if (!(*src & 0xf0)) {
      *q++ = *src++;
    } else  {
      // next four bytes
      int c = (*src++ & 0x07) << 18;
      c |= (*src++ & 0x3f) << 12;
      c |= (*src++ & 0x3f) << 6;
      c |= *src++ & 0x3f;

      *q++ = 0xed;
      *q++ = 0xa0 | ((c & 0x1f0000) >> 16) - 1;
      *q++ = 0x80 | ((c & 0xfc00) >> 10);

      *q++ = 0xed;
      *q++ = 0xb0 | ((c & 0x3c0) >> 6);
      *q++ = 0x80 | c & 0x3f;
    }
  }
  *q = 0;

  *l = q - str;
  return str;
}


static inline bool utf8_contains_unicode(const char *s)
{
  while (*s) {
    if (*s++ & (1 << 7)) {
      return true;
    }
  }
  return false;
}


static int regex_closure(lua_State *lstate)
{
  uint8_t *capture[CAPTURE_COUNT_MAX * 2];

  struct regex *r = lua_touserdata(lstate, lua_upvalueindex(1));
  const int global = lre_get_flags(r->bc) & LRE_FLAG_GLOBAL;
  const int named_groups = lre_get_flags(r->bc) & LRE_FLAG_NAMED_GROUPS;
  const int capture_count = lre_get_capture_count(r->bc);

  const uint8_t *input = (uint8_t *) luaL_checkstring(lstate, 1);
  const int input_len = strlen((char *) input);

  // index of the (first) corresponding char in the utf8 string
  uint32_t *indices = malloc((input_len+1) * sizeof *indices);
  uint16_t *input_utf16 = malloc((input_len+1) * sizeof *input_utf16);

  /* { */
  /*   fprintf(stderr, "--- input:\n"); */
  /*   const uint8_t *c = input; */
  /*   while (*c) { */
  /*     fprintf(stderr, "%hhx : %u\n", *c, *c); */
  /*     c++; */
  /*   } */
  /* } */

  // convert input string to utf16
  int input_utf16_len = 0;
  {
    const uint8_t *pos = (const uint8_t *) input;
    while (*pos) {
      indices[input_utf16_len] = pos - input;
      int c = unicode_from_utf8(pos, UTF8_CHAR_LEN_MAX, &pos);
      if ((unsigned) c > 0xffff) {
        input_utf16[input_utf16_len++] = (((c - 0x10000) >> 10) | (0xd8 << 8));
        input_utf16[input_utf16_len++] = (c & 0xfffff) | (0xdc << 8);
      } else {
        input_utf16[input_utf16_len++] = c & 0xffff;
      }
    }
    input_utf16[input_utf16_len] = 0;
    indices[input_utf16_len] = input_len;
  }

  /* { */
  /*   const uint16_t *c = input_utf16; */
  /*   fprintf(stderr, "--- input UTF16:\n"); */
  /*   while (*c) { */
  /*     const uint8_t *chars = (uint8_t*) c; */
  /*     fprintf(stderr, "%02hhx %02hhx : %u\n", *chars, *(chars+1), *c); */
  /*     c++; */
  /*   } */
  /* } */

  lua_newtable(lstate);

  int nmatch = 0;

  int cindex = 0;
  while (lre_exec(capture, r->bc, (uint8_t *) input_utf16, cindex, input_utf16_len, 1, NULL) == 1) {

    if (capture[0] == capture[1]) {
      // empty match -> continue matching from next character (to prevent an endless loop).
      // This is basically the same implementation as in quickjs, see
      // https://github.com/bellard/quickjs/blob/2788d71e823b522b178db3b3660ce93689534e6d/quickjs.c#L42857-L42869

      // +1 works for ascii, take a closer look for unicode
      cindex++;
    } else {
      cindex = (capture[1] - (uint8_t *) input_utf16) / 2;
    }

    lua_newtable(lstate);

    lua_pushnumber(lstate, indices[(capture[0] - (uint8_t *) input_utf16) / 2]);
    lua_setfield(lstate, -2, "begin_ind");

    lua_pushnumber(lstate, indices[(capture[1] - (uint8_t *) input_utf16) / 2]);
    lua_setfield(lstate, -2, "end_ind");

    /* lua_pushnumber(lstate, capture[1] - capture[0]); */
    /* lua_setfield(lstate, -2, "length"); */

    lua_newtable(lstate);

    const char* group_names = NULL;
    if (named_groups) {
      lua_newtable(lstate);
      group_names = lre_get_groupnames(r->bc);
    }
    for (int i = 1; i < capture_count; i++) {
      uint16_t a = indices[(capture[2*i] - (uint8_t *) input_utf16) / 2];
      uint16_t b = indices[(capture[2*i+1] - (uint8_t *) input_utf16) / 2];
      lua_pushlstring(lstate, (char *) input+a, b-a);
      lua_rawseti(lstate, -2, i);
      if (named_groups && group_names != NULL) {
        if (*group_names != '\0') { // check if current group is named
          lua_pushlstring(lstate, (char *) capture[2 * i], capture[2 * i + 1] - capture[2 * i]);
          lua_setfield(lstate, -3, group_names);
          group_names += strlen(group_names) + 1;  // move to the next group name
        } else {
          group_names += 1; // move to the next group name
        }
      } 
    }

    if (named_groups) {
      lua_setfield(lstate, -3, "groups");
      lua_setfield(lstate, -2, "named_groups");
    } else {
      lua_setfield(lstate, -2, "groups");
    }

    lua_rawseti(lstate, -2, ++nmatch);

    if (!global || cindex > input_utf16_len) {
      break;
    }
  }

  free(input_utf16);
  free(indices);

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
        case 'n': re_flags |= LRE_FLAG_NAMED_GROUPS; break;
        default: /* unknown flag */;
      }
    }
  }

  uint8_t *bc;

  if (utf8_contains_cesu8(regex)) {
    int l;
    char *regex_cesu8 = utf8_to_cesu8(regex, &l);
    bc = lre_compile(&len, error_msg, sizeof error_msg, regex_cesu8,
        l, re_flags, NULL);
    free(regex_cesu8);
  } else {
    bc = lre_compile(&len, error_msg, sizeof error_msg, regex,
        strlen(regex), re_flags, NULL);
  }

  if (!bc) {
    lua_pushnil(lstate);
    lua_pushstring(lstate, error_msg);
    return 2;
  }

  struct regex *ud = lua_newuserdata(lstate, sizeof *ud);
  ud->bc = bc;


  luaL_getmetatable(lstate, "jsregexp_meta");
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
  new_lib(lstate, jsregexp_lib);
  luaL_newmetatable(lstate, "jsregexp_meta");
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(lstate, jsregexp_meta, 0);
#else
    luaL_register(lstate, NULL, jsregexp_meta);
#endif
  lua_pop(lstate, 1);
  return 1;
}
