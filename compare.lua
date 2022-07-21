local jsregexp = require("jsregexp")
local jsregexp_ffi = require("jsregexp_ffi")

local function time(f, ...)
	local t0 = os.clock()
	f(...)
	local t1 = os.clock()
	return t1 - t0
end

local function compare(regex, flags, str, reps)
	reps = reps or 1000000

	print(regex, flags, str)

	local reg = jsregexp.compile(regex, flags)
	local reg_ffi = jsregexp_ffi.compile(regex, flags)

	local c = time(function(n)
		local str = str
		for _ = 1,n do
			reg(str)
		end
	end, reps)

	local ffi = time(function(n)
		local str = str
		for _ = 1,n do
			reg_ffi(str)
		end
	end, reps)

	print((c-ffi)/c)
end

compare("a", "", "Hello World")
compare("(\\w)\\w*", "g", "Hello World")
compare("^(.*)\\.(.*)$", "g", "asdf8ziushdfiuaszef987123r.txt")
compare("(.)(.)(.)(.)(.)(.)(.)", "g", "asdf8ziushdfiuaszef987123r.txt")
compare("(a*)*c", "", "aaaaaa")
