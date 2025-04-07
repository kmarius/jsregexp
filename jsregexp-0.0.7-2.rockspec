package = "jsregexp"
version = "0.0.7-2"
source = {
	url = "git://github.com/kmarius/jsregexp.git",
	tag = "v0.0.7",
}
description = {
	summary = "javascript (ECMA19) regular expressions for lua",
	detailed = [[
Provides ECMAScript regular expressions for Lua 5.1, 5.2, 5.3, 5.4 and LuaJit. Uses libregexp from Fabrice Bellard's QuickJS.
	]],
	homepage = "https://github.com/kmarius/jsregexp",
	license = "MIT",
}
dependencies = { "lua >= 5.1" }
build = {
	type = "builtin",
	modules = {
		["jsregexp.core"] = {
			"jsregexp.c",
			"libregexp/cutils.c",
			"libregexp/libregexp.c",
			"libregexp/libunicode.c",
		},
		jsregexp = "jsregexp.lua",
	},
}
