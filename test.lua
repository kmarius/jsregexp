local jsregexp = require("jsregexp")
local unpack = unpack or table.unpack

local tests = 0
local fails = 0
local successes = 0

-- all other tests count not-compilation as failure
local function test_compile(str, regex, flags, want)
	local function fail(...)
		print(str, regex, flags, want)
		print(...)
		fails = fails + 1
	end
	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if not r and want then
		return fail("compilation error")
	elseif r and not want then
		return fail("should not compile")
	end
	successes = successes + 1
end

local function test_call(str, regex, flags, want)
	local function fail(...)
		print(str, regex, flags, want)
		print(...)
		fails = fails + 1
	end

	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if not r then
		return fail("compilation error")
	end
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
			for k, v in pairs(want.named_groups) do
				if val.named_groups[k] ~= v then
					fails = fails + 1
					print(
						string.format(
							"named group mismatch group '%s': expected '%s', actual '%s'",
							k,
							v,
							val.named_groups[k]
						)
					)
					return
				end
			end
		end
	end
	successes = successes + 1
end

local function test_exec(str, regex, flags, want)
	local function fail(...)
		print(str, regex, flags, want)
		print(...)
		fails = fails + 1
	end

	tests = tests + 1
	local r = jsregexp.compile(regex, flags)
	if not r then
		return fail("compilation error")
	end
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
					return fail(
						string.format("named group %s mismatch, wanted %s, got %s", key, val, match.groups[key])
					)
				end
			end
		end
		if match_wanted.indices and not match.indices then
			return fail("expected indices table")
		end
		if not match_wanted.indices and match.indices then
			return fail("expected no indices table")
		end
		if match_wanted.indices then
			if match_wanted.indices.groups and not match.indices.groups then
				return fail("expected indices.groups table")
			end
			if not match_wanted.indices.groups and match.indices.groups then
				return fail("expected no indices.groups table")
			end
			for i = 0, #match.indices do
				local a, b = unpack(match_wanted.indices[i])
				local c, d = unpack(match.indices[i])
				if a ~= c or b ~= d then
					return fail(
						string.format("wrong indices for group %d, expected {%d, %d}, got {%d, %d}", i, a, b, c, d)
					)
				end
			end
			if match_wanted.indices.groups then
				for key, val in pairs(match_wanted.indices.groups) do
					if not match_wanted.indices.groups[key] then
						return fail(string.format("unexpected key in indices.groups: %s", key))
					end
					local a, b = unpack(match_wanted.indices.groups[key])
					local c, d = unpack(val)
					if a ~= c or b ~= d then
						return fail(
							string.format(
								"wrong indices for group %s, expected {%d, %d}, got {%d, %d}",
								key,
								a,
								b,
								c,
								d
							)
						)
					end
				end
			end
		end
	end
	local match = r:exec(str)
	if r.global and match then
		return fail(string.format("surplus match: %s", match))
	end
	successes = successes + 1
end

local function test_test(str, regex, flags, want)
	local function fail(...)
		print(str, regex, flags, want)
		print(...)
		fails = fails + 1
	end

	tests = tests + 1
	local r = jsregexp.compile(regex, flags)
	if not r then
		return fail("compilation error")
	end
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
end

local function test_match(str, regex, flags, want)
	local function fail(fmt, ...)
		print(str, regex, flags, want)
		print(string.format(fmt, ...))
		fails = fails + 1
	end
	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if not r then
		return fail("compilation error")
	end
	local matches = r:match(str)
	if want and not matches then
		return fail("no matches found")
	end
	if not want and matches then
		return fail("matches found, none wanted")
	end
	if want and matches then
		if #want ~= #matches then
			return fail("number of matches mismatch, wanted %d, got %d", #want, #matches)
		end
		for i, match_want in ipairs(want) do
			local match = matches[i][0]
			if match ~= match_want then
				return fail("match mismatch, wanted %s, got %s", match_want, match)
			end
		end
	end
	successes = successes + 1
end

local function test_match_all_list(str, regex, flags, want)
	local function fail(fmt, ...)
		print(str, regex, flags, want)
		print(string.format(fmt, ...))
		fails = fails + 1
	end
	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if not r then
		return fail("compilation error")
	end
	local matches = r:match_all_list(str)
	if want and not matches then
		return fail("no matches found")
	end
	if not want and matches then
		return fail("matches found, none wanted")
	end
	if want and matches then
		if #want ~= #matches then
			return fail("number of matches mismatch, wanted %d, got %d", #want, #matches)
		end
		for i, match_want in ipairs(want) do
			local match = matches[i][0]
			if match ~= match_want then
				return fail("match mismatch, wanted %s, got %s", match_want, match)
			end
		end
	end
	successes = successes + 1
end

local function test_search(str, regex, flags, want)
	local function fail(fmt, ...)
		print(str, regex, flags, want)
		print(string.format(fmt, ...))
		fails = fails + 1
	end
	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if not r then
		return fail("compilation error")
	end
	local idx = r:search(str)
	if idx ~= want then
		return fail("search failed: wanted %d, got %d", want, idx)
	end
	successes = successes + 1
end

local function test_split(str, regex, flags, want)
	local function fail(fmt, ...)
		print(str, regex, flags, want)
		print(string.format(fmt, ...))
		fails = fails + 1
	end
	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if not r then
		return fail("compilation error")
	end
	local split = r:split(str)
	local min = math.min(#split, #want)
	for i = 1, min do
		local w = want[i]
		if w ~= split[i] then
			return fail("split mismatch, wanted %s, got %s", w, split[i])
		end
	end
	if #split ~= #want then
		return fail("number of splits mismatch, wanted %d, got %d", #want, #split)
	end
	successes = successes + 1
end

local function test_replace(str, regex, flags, replacement, want)
	local function fail(fmt, ...)
		print(str, regex, flags, want)
		print(string.format(fmt, ...))
		fails = fails + 1
	end
	tests = tests + 1
	local r = jsregexp.compile_safe(regex, flags)
	if not r then
		return fail("compilation error")
	end
	local res = r:replace(str, replacement)
	if res ~= want then
		return fail("replacement mismatch, wanted %s, got %s", want, res)
	end
	successes = successes + 1
end

test_compile("dummy", "(.*", "", nil)
test_compile("dummy", "[", "", nil)

-- 0xfd (together with other wird chars) crashes lre_compile if not caught
-- (luajit at least..)
test_compile("dummy", string.char(0xfd, 166, 178, 165, 138, 183), "", nil)

test_call("dummy", ".", "", { { "d" } })
test_call("du", ".", "g", { { "d" }, { "u" } })

test_call("dummy", "c", "", {})
test_call("dummy", "c", "g", {})
test_call("dummy", "d", "", { { "d" } })
test_call("dummy", "m", "", { { "m" } })
test_call("dummy", "m", "g", { { "m" }, { "m" } })

test_call("dummy", "(dummy)", "", { { "dummy", groups = { "dummy" } } })
test_call("The quick brown fox jumps over the lazy dog", "\\w+", "", { { "The" } })
test_call(
	"The quick brown fox jumps over the lazy dog",
	"\\w+",
	"g",
	{ { "The" }, { "quick" }, { "brown" }, { "fox" }, { "jumps" }, { "over" }, { "the" }, { "lazy" }, { "dog" } }
)
test_call("The quick brown fox jumps over the lazy dog", "[aeiou]{2,}", "g", { { "ui" } })

test_call("äöü", ".", "g", { { "ä" }, { "ö" }, { "ü" } })
test_call("äöü", ".", "", { { "ä" } })
test_call("ÄÖÜ", ".", "", { { "Ä" } })
test_call("äöü", "[äöü]", "g", { { "ä" }, { "ö" }, { "ü" } })
test_call("äöü", "[äöü]*", "g", { { "äöü" }, { "" } })
test_call("äÄ", "ä", "gi", { { "ä" }, { "Ä" } })
test_call("öäü.haha", "([^.]*)\\.(.*)", "", { { "öäü.haha", groups = { "öäü", "haha" } } })

test_call("𝄞", "𝄞", "", { { "𝄞" } })
-- these empty matches are expected and consistent with vscode
test_call("öö öö", "ö*", "g", { { "öö" }, { "" }, { "öö" }, { "" } })
test_call("𝄞𝄞 𝄞𝄞", "[^ ]*", "g", { { "𝄞𝄞" }, { "" }, { "𝄞𝄞" }, { "" } })
test_call("𝄞𝄞", "𝄞*", "", { { "𝄞𝄞" } })
-- doesn't work in vscode, matches only a single 𝄞 each time:
test_call("𝄞𝄞𐐷𝄞𝄞", "𝄞*", "g", { { "𝄞𝄞" }, { "" }, { "𝄞𝄞" }, { "" } })
-- vscode actually splits the center unicode character and produces an extra empty match. we don't.
test_call("öö𐐷öö", "ö*", "g", { { "öö" }, { "" }, { "öö" }, { "" } })
test_call("a", "𝄞|a", "g", { { "a" } }) -- utf16 regex, ascii input

test_call("κόσμε", "(κόσμε)", "", { { "κόσμε", groups = { "κόσμε" } } })

test_call(
	"jordbær fløde på",
	"(jordbær fløde på)",
	"",
	{ { "jordbær fløde på", groups = { "jordbær fløde på" } } }
)

test_call(
	"Heizölrückstoßabdämpfung",
	"(Heizölrückstoßabdämpfung)",
	"",
	{ { "Heizölrückstoßabdämpfung", groups = { "Heizölrückstoßabdämpfung" } } }
)

test_call(
	"Fête l'haï volapük",
	"(Fête l'haï volapük)",
	"",
	{ { "Fête l'haï volapük", groups = { "Fête l'haï volapük" } } }
)

test_call(
	"Árvíztűrő tükörfúrógép",
	"(Árvíztűrő tükörfúrógép)",
	"",
	{ { "Árvíztűrő tükörfúrógép", groups = { "Árvíztűrő tükörfúrógép" } } }
)

test_call(
	"いろはにほへとちりぬるを",
	"(いろはにほへとちりぬるを)",
	"",
	{ { "いろはにほへとちりぬるを", groups = { "いろはにほへとちりぬるを" } } }
)

test_call(
	"Съешь же ещё этих мягких французских булок да выпей чаю",
	"(Съешь же ещё этих мягких французских булок да выпей чаю)",
	"",
	{
		{
			"Съешь же ещё этих мягких французских булок да выпей чаю",
			groups = {
				"Съешь же ещё этих мягких французских булок да выпей чаю",
			},
		},
	}
)

-- no idea how thai works
-- test("จงฝ่าฟันพัฒนาวิชาการ", "(จงฝ่าฟันพัฒนาวิชาการ)", "", {{"จงฝ่าฟันพัฒนาวิชาการ", groups="จงฝ่าฟันพัฒนาวิชาการ"}})

-- named groups:
test_call("The quick brown fox jumps over the lazy dog", "(?<first_word>\\w+) (\\w+) (?<third_word>\\w+)", "n", {
	{
		"The quick brown",
		groups = { "The", "quick", "brown" },
		named_groups = { first_word = "The", third_word = "brown" },
	},
})
test_call(
	"The qüick bröwn föx jümps över the lazy dög",
	"(?<first_word>[^ ]+) ([^ ]+) (?<third_word>[^ ]+)",
	"n",
	{
		{
			"The qüick bröwn",
			groups = { "The", "qüick", "bröwn" },
			named_groups = {
				first_word = "The",
				third_word = "bröwn",
			},
		},
	}
)
test_call("The quick bröwn föx", "(?<first_wörd>[^ ]+) ([^ ]+) (?<third_wörd>[^ ]+)", "n", {
	{
		"The quick bröwn",
		groups = { "The", "quick", "bröwn" },
		named_groups = { ["first_wörd"] = "The", ["third_wörd"] = "bröwn" },
	},
})
test_call("𝄞𝄞 𐐷", "(?<word>[^ ]+)", "ng", {
	{ "𝄞𝄞", groups = { "𝄞𝄞" }, named_groups = { word = "𝄞𝄞" } },
	{ "𐐷", groups = { "𐐷" }, named_groups = { word = "𐐷" } },
})

test_exec("The quick brown", "\\w+", "g", { { [0] = "The" }, { [0] = "quick" }, { [0] = "brown" } })
test_exec(
	"The quick brown fox",
	"(\\w+) (\\w+)",
	"g",
	{ { [0] = "The quick", "The", "quick" }, { [0] = "brown fox", "brown", "fox" } }
)
test_exec("The quick brown fox", "(?<word1>\\w+) (\\w+)", "g", {
	{ [0] = "The quick", "The", "quick", groups = { word1 = "The" } },
	{ [0] = "brown fox", "brown", "fox", groups = { word1 = "brown" } },
})

test_exec("The Quick Brown Fox Jumps Over The Lazy Dog", "quick\\s(?<color>brown).+?(jumps)", "di", {
	{
		[0] = "Quick Brown Fox Jumps",
		[1] = "Brown",
		[2] = "Jumps",
		indices = {
			[0] = { 5, 25 },
			[1] = { 11, 15 },
			[2] = { 21, 25 },
			groups = {
				color = { 11, 15 },
			},
		},
		index = 4,
		input = "The Quick Brown Fox Jumps Over The Lazy Dog",
		groups = {
			color = "Brown",
		},
	},
})

test_test("The quick brown", "\\w+", "", { true })
test_test("The quick brown", "\\d+", "", { false })
test_test("The quick brown", "\\w+", "g", { true, true, true })

test_match("The quick brown", "\\d+", "g", nil)
test_match("The quick brown", "\\w+", "g", { "The", "quick", "brown" })

test_match_all_list("The quick brown", "\\d+", "g", {})
test_match_all_list("The quick brown", "\\w+", "g", { "The", "quick", "brown" })

test_search("The quick brown", "nothing", "g", -1)
test_search("The quick brown", "quick", "g", 5)

test_split("abc", "x", "g", { "abc" })
test_split("", "a?", "g", {})
test_split("", "a", "g", { "" })
test_split("1-2-3", "-", "g", { "1", "2", "3" })
test_split("1-2-", "-", "g", { "1", "2", "" })
test_split("-2-3", "-", "g", { "", "2", "3" })
test_split("--", "-", "g", { "", "", "" })
test_split("Hello 1 word. Sentence number 2.", "(\\d)", "g", { "Hello ", "1", " word. Sentence number ", "2", "." })

test_replace("a1b2c", "X", "g", "_", "a1b2c")
test_replace("a1b2c", "\\d", "", "_", "a_b2c")
test_replace("a1b2c", "\\d", "g", "_", "a_b_c")
test_replace("a1b2c", "(\\d)(.)", "g", "$1", "a12")
test_replace("a1b2c", "(\\d)(.)", "g", "$2", "abc")

local bold_green = "\27[1;32m"
local bold_red = "\27[1;31m"
local normal = "\27[0m"

local color = fails == 0 and bold_green or bold_red
print(string.format("%s%d tests run, %d successes, %d failed%s", color, tests, successes, fails, normal))
if fails > 0 then
	collectgarbage()
	os.exit(1)
end
