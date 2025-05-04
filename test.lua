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
		if not match then
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
		if r.global then
			for i, match_want in ipairs(want) do
				local match = matches[i]
				if match ~= match_want then
					return fail("match mismatch, wanted %s, got %s", match_want, match)
				end
			end
		else
			-- TODO: compare match object
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

local function test_replace_all(str, regex, flags, replacement, want)
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

-- 0xfd (together with other weird chars) crashes lre_compile if not caught
-- (luajit at least..)
test_compile("dummy", string.char(0xfd, 166, 178, 165, 138, 183), "", nil)

test_exec("wut", "wot", "", {})
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
test_match_all_list("𝄞𝄞𐐷𝄞𝄞", "𝄞*", "g", { "𝄞𝄞", "", "𝄞𝄞", "" })

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

test_replace("a b", "\\w+", "", "_", "_ b")
test_replace("a b", "\\w+", "", function()
	return "_"
end, "_ b")
test_replace("12 34", "\\d+", "", "_", "_ 34")
test_replace("123 456", "\\d+", "", "_", "_ 456")
test_replace("a1b2c", "X", "g", "_", "a1b2c")
test_replace("a1b2c", "\\d", "", "_", "a_b2c")
test_replace("a1b2c", "\\d", "g", "_", "a_b_c")
test_replace("a1b2c", "(\\d)(.)", "g", "$1", "a12")
test_replace("a1b2c", "(\\d)(.)", "g", "$2", "abc")

test_replace_all("a b", "\\w+", "g", "_", "_ _")
test_replace_all("a b", "\\w+", "g", function()
	return "_"
end, "_ _")
test_replace_all("12 34", "\\d+", "g", "_", "_ _")
test_replace_all("123 456", "\\d+", "g", "_", "_ _")
test_replace_all("a1b2c", "X", "g", "_", "a1b2c")
test_replace_all("a1b2c", "\\d", "g", "_", "a_b_c")
test_replace_all("a1b2c", "\\d", "g", "_", "a_b_c")
test_replace_all("a1b2c", "(\\d)(.)", "g", "$1", "a12")
test_replace_all("a1b2c", "(\\d)(.)", "g", "$2", "abc")

local bold_green = "\27[1;32m"
local bold_red = "\27[1;31m"
local normal = "\27[0m"

local color = fails == 0 and bold_green or bold_red
print(string.format("%s%d tests run, %d successes, %d failed%s", color, tests, successes, fails, normal))
if fails > 0 then
	collectgarbage()
	os.exit(1)
end
