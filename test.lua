local jsregexp = require("jsregexp")

local tests = 0
local fails = 0
local successes = 0

-- TODO: print more info case on fail
local function test(str, regex, flags, want)
	tests = tests + 1
	local r = jsregexp.compile(regex, flags)
	if want and r then
		local res = r(str)
		if #res ~= #want then
			print("match count mismatch: wanted", #want, "got ", #res)
			fails = fails + 1
			return
		end
		for i, val in pairs(res) do
			local want = want[i]
			if not want then
				fails = fails + 1
				return
			end
			local match = string.sub(str, val.begin_ind, val.end_ind)
			if match ~= want[1] then
				fails = fails + 1
				print(match, want[1])
				return
			end
			if #val.groups > 0 then
				if not want.groups or #want.groups ~= #val.groups then
					fails = fails + 1
					return
				end
				for j, v in pairs(val.groups) do
					if v ~= want.groups[j] then
						fails = fails + 1
					end
				end
			else
				if want.groups and #want.groups > 0 then
					fails = fails + 1
					return
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
	else
		fails = fails + 1
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

-- no unicode yet
-- test("äöü", ".", "g", {{"ä"}, {"ö"}, {"ü"}})
-- test("äöü", "[äöü]", "g", {{"ä"}, {"ö"}, {"ü"}})
-- test("äöü", ".", "", {{"ä"}})
-- test("ÄÖÜ", ".", "", {{"Ä"}})

test("dummy", "(dummy)", "", {{"dummy", groups = {"dummy"}}})

test("The quick brown fox jumps over the lazy dog", "\\w+", "", {{"The"}})
test("The quick brown fox jumps over the lazy dog", "\\w+", "g", {{"The"}, {"quick"}, {"brown"}, {"fox"}, {"jumps"}, {"over"}, {"the"}, {"lazy"}, {"dog"}})
test("The quick brown fox jumps over the lazy dog", "[aeiou]{2,}", "g", {{"ui"}})

test("The quick brown fox jumps over the lazy dog", "(?<first_word>\\w+) (\\w+) (?<third_word>\\w+)", "n",
	{{"The quick brown", groups={"The", "quick", "brown"}, named_groups={first_word="The", third_word="brown"}}}
)

local bold_green = "\27[1;32m"
local bold_red = "\27[1;31m"
local normal = "\27[0m"

local color = fails == 0 and bold_green or bold_red
print(string.format("%s%d tests run, %d successes, %d failed%s", color, tests, successes, fails, normal))
collectgarbage()
os.exit(fails == 0 and 0 or 1)
