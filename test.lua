local jsregexp = require("jsregexp")

local tests = 0
local fails = 0
local success = 0

local function test(str, regex, format, flags, want)
	tests = tests + 1
	local replacer, err = jsregexp.compile(regex, format, flags)
	if not replacer then
		if want ~= nil then
			print(string.format('\27[0;31mFailed:\27[m\n str=%s\n regex=%s\n format=%s\n flags=%s\n wanted=%s\n result=%s', str, regex, format, flags, want, err))
			fails = fails + 1
			return
		end
	else
		if want == nil then
			print(string.format('\27[0;31mFailed:\27[m\n str=%s\n regex=%s\n format=%s\n flags=%s\n wanted=compile failure', str, regex, format, flags))
			fails = fails + 1
			return
		end
		local result = replacer(str)
		if result ~= want then
			print(string.format('\27[0;31mFailed:\27[m\n str=%s\n regex=%s\n format=%s\n flags=%s\n wanted=%s\n result=%s', str, regex, format, flags, want, result))
			fails = fails + 1
			-- print("", "result", "", "want")
			-- for i = 1, #result > #want and #result or #want do
			-- 	local c1 = result:sub(i,i)
			-- 	local c2 = want:sub(i,i)
			-- 	print(i, c1, c1:byte(), c2, c2:byte())
			-- end
			return
		end
	end
	success = success + 1
end

test("dummy", "(.*", "dummy", "", nil)
test("dummy", "[", "dummy", "", nil)
test("dummy", ".", "$", "", nil)
test("dummy", ".", "$ ", "", nil)
test("dummy", ".", "$a", "", nil)
test("dummy", ".", "${", "", nil)
test("dummy", ".", "${}", "", nil)
test("dummy", ".", "${ ", "", nil)
test("dummy", ".", "${ 1}", "", nil)
test("dummy", ".", "${1 }", "", nil)
test("dummy", ".", "${1", "", nil)
test("dummy", ".", "${-1", "", nil)

test("", "(\\s)|(\\w*)", "${1:+_}${2:/upcase}", "g", "")
test("shouldn't hang", "(\\w*)|shouldn't hang", "$1", "g", "shouldn't hang")

test("dummy", "(.)", "${112837649182736541987325418976325417653120835641027}", "", "ummy")
test("dummy", "(.)", "${20}", "", "ummy")
test("dummy", "(.)", "$1", "", "dummy")
test("dummy", "(dummy)", "you ${1} yo", "", "you dummy yo")
test("dummy", "(dummy)", "you $1 yo", "", "you dummy yo")
test("dummy", "dummy", "you $0 yo", "", "you dummy yo")
test("dummy", "dummy", "you ${0} yo", "", "you dummy yo")

test("dummy", "(\\d*).*", "a${1:ha\\", "", nil)
test("dummy", "(\\d*).*", "a${1:ha\\}", "", nil)
test("dummy", "(\\d*).*", "${1:-ha}", "", "ha")
test("dummy", "(\\d*).*", "${1:ha}", "", "ha")
test("dummy", "(\\d*).*", "a${1:-ha}b", "", "ahab")
test("dummy", "(\\d*).*", "a${1:ha}b", "", "ahab")
test("dummy", "(\\d*).*", "a${1:ha\\}}b", "", "aha}b")

test("dummy", ".", "${1:+abc", "", nil)
test("dummy", ".", "${1:+", "", nil)
test("dummy", ".", "${1:+\\", "", nil)
test("dummy", ".", "${1:+\\}", "", nil)
test("dummy", "(\\d*).*", "${1:+}", "", "")
test(".dummy.", "(\\w+)", "a${1:+}b", "", ".ab.")

test("dummy", ".", "${1:?", "", nil)
test("dummy", ".", "${1:?:", "", nil)
test("dummy", ".", "${1:?:abc", "", nil)
test("dummy", ".", "${1:?abc", "", nil)
test("dummy", ".", "${1:?abc:", "", nil)
test("dummy", ".", "${1:?abc:def", "", nil)
test("dummy", ".", "${1:?abc}", "", nil)
test("dummy", "(.+)", "a${1:?yep:nope}b", "", "ayepb")
test("dummy", "(.+)", "${1:?yep:nope}", "", "yep")
test("dummy", "(\\d*).*", "${1:?yep:nope}", "", "nope")
test("dummy", "(.*)", "${1:?yep:}", "", "yep")
test("dummy", "(\\d*).*", "${1:?:nope}", "", "nope")

test("dummy", ".", "${1:/yooo}", "", nil)
test("dummy", ".", "${1:/upcase }", "", nil)
test("dummy", ".", "${1:/upcase", "", nil)
test("dumMY", "(.*)", "a${1:/upcase}b", "", "aDUMMYb")
test("dumMY", "(.*)", "a${1:/upcase}b", "", "aDUMMYb")
test("dumMY", "(.*)", "a${1:/downcase}b", "", "adummyb")
test("aNOTher duMMy", "(.*)", "a${1:/capitalize}b", "", "aAnother Dummyb")
test("another dummy", "(\\w+)", "${1:/upcase}", "g", "ANOTHER DUMMY")
test("121212", "1", "a", "g", "a2a2a2")

test("d", ".", "\\$", "", "$")
test("d", ".", "\\${1:yee}", "", "${1:yee}")
test("d", ".", "\\$}", "", "$}")
test("d", ".", "\\\\", "", "\\")

test("strING STRING string", "(string)", "${1:+match}", "g", "strING STRING match")
test("strING STRING string", "(string)", "${1:+match}", "gi", "match match match")

local bold_green = "\27[1;32m"
local bold_red = "\27[1;31m"
local normal = "\27[0m"

local color = fails == 0 and bold_green or bold_red
print(string.format("%s%d tests run, %d successes, %d failed", color, tests, success, fails), normal)
os.exit(fails == 0 and 0 or 1)
