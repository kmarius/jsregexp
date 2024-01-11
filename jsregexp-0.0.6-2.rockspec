package = "jsregexp"
version = "0.0.6-2"
source = {
    url = "git://github.com/kmarius/jsregexp.git",
    branch = "master",
    tag = "v0.0.6"
}
description = {
    summary = "javascript (ECMA19) regular expressions for lua",
    detailed = [[
WIP: This library offers a single function to use javascript regular expressions in lua. It makes use of libregexp from https://bellard.org/quickjs/.
	]],
    homepage = "https://github.com/kmarius/jsregexp",
    license = "MIT"
}
dependencies = {"lua >= 5.1"}
build = {
    type = "builtin",
    modules = {
        ["jsregexp.core"] = {
            "jsregexp.c", "libregexp/cutils.c", "libregexp/libregexp.c", "libregexp/libunicode.c"
        },
        jsregexp = "jsregexp.lua"
    }
}