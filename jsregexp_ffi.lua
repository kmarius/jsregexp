local ffi = require("ffi")

ffi.cdef [[
uint8_t *lre_compile(int *plen, char *error_msg, int error_msg_size,
	const char *buf, size_t buf_len, int re_flags,
	void *opaque);
int lre_get_capture_count(const uint8_t *bc_buf);
int lre_get_flags(const uint8_t *bc_buf);
int lre_exec(uint8_t **capture,
	const uint8_t *bc_buf, const uint8_t *cbuf, int cindex, int clen,
	int cbuf_type, void *opaque);
]]
local LRE_FLAG_GLOBAL = bit.lshift(1, 0)
local LRE_FLAG_IGNORECASE = bit.lshift(1, 1)

local lre = ffi.load("regexp")

local M = {}

function M.compile(regex, flags)
	local buf = ffi.new("char[?]", 64)
	local len = ffi.new("int[1]", 0)
	local re_flags = 0

	if flags:find("g") then
		re_flags = bit.bor(re_flags, LRE_FLAG_GLOBAL)
	end
	if flags:find("i") then
		re_flags = bit.bor(re_flags, LRE_FLAG_IGNORECASE)
	end

	local bc = lre.lre_compile(len, buf, 64, regex, #regex, re_flags, nil)

	if bc == nil then
		return nil, ffi.string(buf, 64)
	end

	local captures = ffi.new("uint8_t*[?]", 255 * 2)
	return function(input)
		local t = {}

		local global = bit.band(lre.lre_get_flags(bc), LRE_FLAG_GLOBAL) ~= 0
		local capture_count = lre.lre_get_capture_count(bc)

		local input_len = #input
		local inptr = ffi.cast("uint8_t *", input)

		local nmatch = 0

		local cindex = 0
		while lre.lre_exec(captures, bc, input, cindex, input_len, 0, nil) == 1 do
			if captures[0] == captures[1] then
				cindex = cindex + 1
			else
				cindex = captures[1] - inptr;
			end

			local m = {}
			m.begin_ind = 1 + captures[0] - inptr
			m.end_ind = 1 + captures[1] - inptr
			m.length = captures[1] - captures[0]

			local groups = {}
			for i = 1,capture_count-1 do
				groups[i] = ffi.string(captures[2*i], captures[2*i+1] - captures[2*i])
			end
			m.groups = groups

			nmatch = nmatch + 1
			t[nmatch] = m
			if not global or cindex > input_len then
				break;
			end
		end

		return t
	end
end

return M
