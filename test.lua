local jsregexp = require("jsregexp")

local tests = 0
local fails = 0
local success = 0

local function test(str, regex, format, flags, want)
	tests = tests + 1
	local r = jsregexp.compile(regex, flags)
	if r then
		local res = r(str)
		for _, val in pairs(res) do
			print("begin", val.begin)
			print("_end", val._end)
			print("length", val.length)
			for k, v in pairs(val.groups) do
				print(k, v)
			end
		end
		success = success + 1
	end
end

test(" ab yyy ac y ", "a([bc]) (y+)", "", "g", nil)

--
-- test("dummy", "(.*", "dummy", "", nil)
-- test("dummy", "[", "dummy", "", nil)
-- test("dummy", ".", "$", "", nil)
-- test("dummy", ".", "$ ", "", nil)
-- test("dummy", ".", "$a", "", nil)
-- test("dummy", ".", "${", "", nil)
-- test("dummy", ".", "${}", "", nil)
-- test("dummy", ".", "${ ", "", nil)
-- test("dummy", ".", "${ 1}", "", nil)
-- test("dummy", ".", "${1 }", "", nil)
-- test("dummy", ".", "${1", "", nil)
-- test("dummy", ".", "${-1", "", nil)
--
-- test("", "(\\s)|(\\w*)", "${1:+_}${2:/upcase}", "g", "")
-- test("shouldn't hang", "(\\w*)|shouldn't hang", "$1", "g", "shouldn't hang")
-- test("shouldn't hang", "(\\s*)|shouldn't hang", "$1", "g", "shouldn't hang")
--
-- test("ö", "ö", "", "", "")
-- test("äöü", "a", "", "", "äöü")
-- test("äöü", "(.*)", "${1:/upcase}", "", "ÄÖÜ")
-- test("ÄÖÜ", "(.*)", "${1:/downcase}", "", "äöü")
--
-- test("dummy", "c", "", "", "dummy")
-- test("dummy", "c", "", "g", "dummy")
-- test("dummy", "d", "", "", "ummy")
-- test("dummy", "d", "b", "", "bummy")
-- test("dummy", "m", "_", "", "du_my")
-- test("dummy", "m", "_", "g", "du__y")
--
-- test("dummy", "(.)", "${112837649182736541987325418976325417653120835641027}", "", "ummy")
-- test("dummy", "(.)", "${20}", "", "ummy")
-- test("dummy", "(.)", "$1", "", "dummy")
-- test("dummy", "(dummy)", "you ${1} yo", "", "you dummy yo")
-- test("dummy", "(dummy)", "you $1 yo", "", "you dummy yo")
-- test("dummy", "dummy", "you $0 yo", "", "you dummy yo")
-- test("dummy", "dummy", "you ${0} yo", "", "you dummy yo")
--
-- test("dummy", "(\\d*).*", "a${1:ha\\", "", nil)
-- test("dummy", "(\\d*).*", "a${1:ha\\}", "", nil)
-- test("dummy", "(\\d*).*", "${1:-ha}", "", "ha")
-- test("dummy", "(\\d*).*", "${1:ha}", "", "ha")
-- test("dummy", "(\\d*).*", "a${1:-ha}b", "", "ahab")
-- test("dummy", "(\\d*).*", "a${1:ha}b", "", "ahab")
-- test("dummy", "(\\d*).*", "a${1:ha\\}}b", "", "aha}b")
--
-- test("dummy", ".", "${1:+abc", "", nil)
-- test("dummy", ".", "${1:+", "", nil)
-- test("dummy", ".", "${1:+\\", "", nil)
-- test("dummy", ".", "${1:+\\}", "", nil)
-- test("dummy", "(\\d*).*", "${1:+}", "", "")
-- test(".dummy.", "(\\w+)", "a${1:+}b", "", ".ab.")
--
-- test("dummy", ".", "${1:?", "", nil)
-- test("dummy", ".", "${1:?:", "", nil)
-- test("dummy", ".", "${1:?:abc", "", nil)
-- test("dummy", ".", "${1:?abc", "", nil)
-- test("dummy", ".", "${1:?abc:", "", nil)
-- test("dummy", ".", "${1:?abc:def", "", nil)
-- test("dummy", ".", "${1:?abc}", "", nil)
-- test("dummy", "(.+)", "a${1:?yep:nope}b", "", "ayepb")
-- test("dummy", "(.+)", "${1:?yep:nope}", "", "yep")
-- test("dummy", "(\\d*).*", "${1:?yep:nope}", "", "nope")
-- test("dummy", "(.*)", "${1:?yep:}", "", "yep")
-- test("dummy", "(\\d*).*", "${1:?:nope}", "", "nope")
-- test("dummy", "(\\d+)", "${1:?yep:nope} ${1:-man} ${2:+ha}", "", "nope man ") -- no match, the whole string gets replaced with the format where each 'else' branch is used
-- test("dummy", "(\\d*)", "${1:?yep:nope }", "", "nope dummy") -- zero length string gets replaced at the beginning of "dummy"
--
-- test("dummy", ".", "${1:/yooo}", "", nil)
-- test("dummy", ".", "${1:/upcase }", "", nil)
-- test("dummy", ".", "${1:/upcase", "", nil)
-- test("dumMY", "(.*)", "a${1:/upcase}b", "", "aDUMMYb")
-- test("dumMY", "(.*)", "a${1:/upcase}b", "", "aDUMMYb")
-- test("dumMY", "(.*)", "a${1:/downcase}b", "", "adummyb")
-- test("aNOTher duMMy", "(.*)", "a${1:/capitalize}b", "", "aAnother Dummyb")
-- test("another dummy", "(\\w+)", "${1:/upcase}", "g", "ANOTHER DUMMY")
-- test("121212", "1", "a", "g", "a2a2a2")
--
-- test("d", ".", "\\$", "", "$")
-- test("d", ".", "\\${1:yee}", "", "${1:yee}")
-- test("d", ".", "\\$}", "", "$}")
-- test("d", ".", "\\\\", "", "\\")
--
-- test("strING STRING string", "(string)", "${1:+match}", "g", "strING STRING match")
-- test("strING STRING string", "(string)", "${1:+match}", "gi", "match match match")

local bold_green = "\27[1;32m"
local bold_red = "\27[1;31m"
local normal = "\27[0m"

local color = fails == 0 and bold_green or bold_red
print(string.format("%s%d tests run, %d successes, %d failed%s", color, tests, success, fails, normal))
collectgarbage()
os.exit(fails == 0 and 0 or 1)
