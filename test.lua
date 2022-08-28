local jsregexp = require("jsregexp")

local tests = 0
local fails = 0
local successes = 0

-- TODO: print more info case on fail
local function test(str, regex, flags, want)
	local function fail(...)
		print(str, regex, flags, want)
		print(...)
		fails = fails + 1
	end

	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if want and r then
		local res = r(str)
		if #res ~= #want then
			return fail("match count mismatch: wanted", #want, "got ", #res)
		end
		for i, val in pairs(res) do
			local want = want[i]
			if not want then
				return fail("compilation should have failed")
			end
			local match = string.sub(str, val.begin_ind, val.end_ind)
			if match ~= want[1] then
				return fail("global mismatch:", match, want[1])
			end
			if #val.groups > 0 then
				if not want.groups or #want.groups ~= #val.groups then
					return fail("number of match groups mismatch")
				end
				for j, v in pairs(val.groups) do
					if v ~= want.groups[j] then
						return fail("match group mismatch", i, v, want.groups[j])
					end
				end
			else
				if want.groups and #want.groups > 0 then
					return fail("number of match groups mismatch")
				end
			end
			if want.named_groups ~= nil then
				if val.named_groups == nil then
					fails = fails + 1
					return
				end
				for k,v in pairs(want.named_groups) do
					if val.named_groups[k] ~= v then
						fails = fails + 1
						print(string.format("named group mismatch group '%s': expected '%s', actual '%s'", k, v, val.named_groups[k]))
						return
					end
				end
			end
		end
		successes = successes + 1
	elseif not want and not r then
		successes = successes + 1
	elseif not r and want then
		return fail("compilation error")
	else
		return fail("should not compile")
	end
end

local function test_exec(str, regex, flags, want)
	local function fail(...)
		print(str, regex, flags, want)
		print(...)
		fails = fails + 1
	end

	tests = tests + 1
	local r = jsregexp.compile(regex, flags)
	if want and r then
		for _, match_wanted in ipairs(want) do
			local match = r:exec(str)
			if match and not match_wanted then
				return fail(string.format("no match expected, got %s", match))
			end
			if not match and match_wanted then
				return fail(string.format("match expected, wanted %s", match_wanted))
			end
			if #match_wanted ~= #match then
				return fail(string.format("match group count mismatch, wanted: %d, got: %d", #match_wanted, #match))
			end
			if match_wanted[0] ~= match[0] then
				return fail(string.format("global mismatch, wanted: %s, got: %s", match_wanted[0], match[0]))
			end
			for i, val in ipairs(match_wanted) do
				if val ~= match[i] then
					return fail(string.format("group %d mismatch, wanted: %s, got: %s", i, match_wanted[i], match[i]))
				end
			end
			if match_wanted.groups and not match.groups then
				return fail("expected named groups")
			end
			if not match_wanted.groups and match.groups then
				return fail("expected no named groups")
			end
			if match_wanted.groups then
				for key, val in pairs(match_wanted.groups) do
					if val ~= match.groups[key] then
						return fail(string.format("named group %s mismatch, wanted %s, got %s", key, val, match.groups[key]))
					end
				end
			end
		end
		local match = r:exec(str)
		if r.global and match then
			return fail(string.format("surplus match: %s", match))
		end
		successes = successes + 1
	elseif not want and not r then
		successes = successes + 1
	elseif not r and want then
		return fail("compilation error")
	else
		return fail("should not compile")
	end
end

local function test_test(str, regex, flags, want)
	local function fail(...)
		print(str, regex, flags, want)
		print(...)
		fails = fails + 1
	end

	tests = tests + 1
	local r = jsregexp.compile(regex, flags)
	if want and r then
		for _, match_wanted in ipairs(want) do
			local match = r:test(str)
			if match ~= match_wanted then
				return fail(string.format("test mismatch, wanted: %s, got: %s", match_wanted, match))
			end
		end
		local match = r:test(str)
		if r.global and match then
			return fail(string.format("surplus match: %s", tostring(match)))
		end
		successes = successes + 1
	elseif not want and not r then
		successes = successes + 1
	elseif not r and want then
		return fail("compilation error")
	else
		return fail("should not compile")
	end
end


test("dummy", "(.*", "", nil)
test("dummy", "[", "", nil)

test("dummy", ".", "", {{"d"}})
test("du", ".", "g", {{"d"}, {"u"}})

test("dummy", "c", "", {})
test("dummy", "c", "g", {})
test("dummy", "d", "", {{"d"}})
test("dummy", "m", "", {{"m"}})
test("dummy", "m", "g", {{"m"}, {"m"}})

test("dummy", "(dummy)", "", {{"dummy", groups = {"dummy"}}})
test("The quick brown fox jumps over the lazy dog", "\\w+", "", {{"The"}})
test("The quick brown fox jumps over the lazy dog", "\\w+", "g", {{"The"}, {"quick"}, {"brown"}, {"fox"}, {"jumps"}, {"over"}, {"the"}, {"lazy"}, {"dog"}})
test("The quick brown fox jumps over the lazy dog", "[aeiou]{2,}", "g", {{"ui"}})

test("äöü", ".", "g", {{"ä"}, {"ö"}, {"ü"}})
test("äöü", ".", "", {{"ä"}})
test("ÄÖÜ", ".", "", {{"Ä"}})
test("äöü", "[äöü]", "g", {{"ä"}, {"ö"}, {"ü"}})
test("äöü", "[äöü]*", "g", {{"äöü"}, {""}})
test("äÄ", "ä", "gi", {{"ä"}, {"Ä"}})
test("öäü.haha", "([^.]*)\\.(.*)", "", {{"öäü.haha", groups={"öäü", "haha"}}})

test("𝄞", "𝄞(", "", nil)
test("𝄞", "𝄞", "", {{"𝄞"}})
-- these empty matches are expected and consistent with vscode
test("öö öö", "ö*", "g", {{"öö"}, {""}, {"öö"}, {""}})
test("𝄞𝄞 𝄞𝄞", "[^ ]*", "g", {{"𝄞𝄞"}, {""}, {"𝄞𝄞"}, {""}})
test("𝄞𝄞", "𝄞*", "", {{"𝄞𝄞"}})
-- doesn't work in vscode, matches only a single 𝄞 each time:
test("𝄞𝄞𐐷𝄞𝄞", "𝄞*", "g", {{"𝄞𝄞"}, {""}, {"𝄞𝄞"}, {""}})
-- vscode actually splits the center unicode character and produces an extra empty match. we don't.
test("öö𐐷öö", "ö*", "g", {{"öö"}, {""}, {"öö"}, {""}})
test("a", "𝄞|a", "g", {{"a"}}) -- utf16 regex, ascii input

test("κόσμε", "(κόσμε)", "", {{"κόσμε", groups={"κόσμε"}}})

test("jordbær fløde på", "(jordbær fløde på)", "", {{"jordbær fløde på", groups={"jordbær fløde på"}}})

test("Heizölrückstoßabdämpfung", "(Heizölrückstoßabdämpfung)", "", {{"Heizölrückstoßabdämpfung", groups={"Heizölrückstoßabdämpfung"}}})

test("Fête l'haï volapük", "(Fête l'haï volapük)", "", {{"Fête l'haï volapük", groups={"Fête l'haï volapük"}}})

test("Árvíztűrő tükörfúrógép", "(Árvíztűrő tükörfúrógép)", "", {{"Árvíztűrő tükörfúrógép", groups={"Árvíztűrő tükörfúrógép"}}})

test("いろはにほへとちりぬるを", "(いろはにほへとちりぬるを)", "", {{"いろはにほへとちりぬるを", groups={"いろはにほへとちりぬるを"}}})

test("Съешь же ещё этих мягких французских булок да выпей чаю", "(Съешь же ещё этих мягких французских булок да выпей чаю)", "", {{"Съешь же ещё этих мягких французских булок да выпей чаю", groups={"Съешь же ещё этих мягких французских булок да выпей чаю"}}})

-- no idea how thai works
-- test("จงฝ่าฟันพัฒนาวิชาการ", "(จงฝ่าฟันพัฒนาวิชาการ)", "", {{"จงฝ่าฟันพัฒนาวิชาการ", groups="จงฝ่าฟันพัฒนาวิชาการ"}})

-- 0xfd (together with other wird chars) crashes lre_compile if not caught
-- (luajit at least..)
test("dummy", string.char(0xfd, 166, 178, 165, 138, 183), "", nil)


-- named groups:
test("The quick brown fox jumps over the lazy dog", "(?<first_word>\\w+) (\\w+) (?<third_word>\\w+)", "n",
{{"The quick brown", groups={"The", "quick", "brown"}, named_groups={first_word="The", third_word="brown"}}}
)
test("The qüick bröwn föx jümps över the lazy dög", "(?<first_word>[^ ]+) ([^ ]+) (?<third_word>[^ ]+)", "n",
{{"The qüick bröwn", groups={"The", "qüick", "bröwn"}, named_groups={first_word="The", third_word="bröwn"}}}
)
test("The quick bröwn föx", "(?<first_wörd>[^ ]+) ([^ ]+) (?<third_wörd>[^ ]+)", "n",
{{"The quick bröwn", groups={"The", "quick", "bröwn"}, named_groups={["first_wörd"]="The", ["third_wörd"]="bröwn"}}}
)
test("𝄞𝄞 𐐷", "(?<word>[^ ]+)", "ng", {{"𝄞𝄞", groups={"𝄞𝄞"}, named_groups={word="𝄞𝄞"}}, {"𐐷", groups={"𐐷"}, named_groups={word="𐐷"}}})

test_exec("The quick brown", "\\w+", "g", {{[0]="The"}, {[0]="quick"}, {[0]="brown"}})
test_exec("The quick brown fox", "(\\w+) (\\w+)", "g", {{[0]="The quick", "The", "quick"}, {[0]="brown fox", "brown", "fox"}})
test_exec("The quick brown fox", "(?<word1>\\w+) (\\w+)", "g",
{{[0]="The quick", "The", "quick", groups={word1="The"}}, {[0]="brown fox", "brown", "fox", groups={word1="brown"}}})

test_test("The quick brown", "\\w+", "", {true})
test_test("The quick brown", "\\d+", "", {false})
test_test("The quick brown", "\\w+", "g", {true, true, true})

local bold_green = "\27[1;32m"
local bold_red = "\27[1;31m"
local normal = "\27[0m"

local color = fails == 0 and bold_green or bold_red
print(string.format("%s%d tests run, %d successes, %d failed%s", color, tests, successes, fails, normal))
if fails > 0 then
	collectgarbage()
	os.exit(1)
end
