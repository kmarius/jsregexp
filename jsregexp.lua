local jsregexp = require "jsregexp.core"

setmetatable(jsregexp, {
    __call = function(self, expr, flags) return jsregexp.compile(expr, flags) end
})

function jsregexp.mt.match(re, str)
    local jstr = jsregexp.to_jsstring(str)
    if not re.global then return re:exec(jstr) end
    local matches = {}
    local val

    re.last_index = 1

    while true do
        val = re:exec(jstr)
        if val == nil then break end
        table.insert(matches, val)
        if #val[0] == 0 then re.last_index = re.last_index + 1 end
    end
    if #matches == 0 then return nil end
    return matches
end

function jsregexp.mt.match_all(re, str)
    -- must duplicate (according to string.proptype.matchAll spec)
    local re2 = jsregexp.compile(re.source, re.flags)
    local jstr = jsregexp.to_jsstring(str)
    return function() return re2:exec(jstr) end
end

function jsregexp.mt.match_all_list(re, str)
    local matches = {}
    for match in jsregexp.mt.match_all(re, str) do table.insert(matches, match) end
    return matches
end

function jsregexp.mt.search(re, str)
    -- spec says to start at 1 and restore last_index
    local prev_last_index = re.last_index
    re.last_index = 1
    local match = re:exec(str)
    re.last_index = prev_last_index
    if match == nil then return -1 end
    return match.index
end

function jsregexp.mt.split(re, str, limit)
    if limit == nil then limit = math.huge end
    if limit == 0 then return {} end
    assert(limit >= 0, "limit must be non-negative")

    local jstr = jsregexp.to_jsstring(str)
    local re2 = jsregexp.compile(re.source, re.flags .. "y") -- add sticky

    local count = 0
    local split = {}
	local prev_index = 1
    while count < limit do
		local li = re2.last_index
        local match = re2:exec(jstr)
		if match then
			if #str == 0 then
				break
			end
			local sub = string.sub(str, prev_index, match.index - 1)
			if #sub > 0 or #match[0] > 0 then table.insert(split, sub) end
			for _, group in ipairs(match) do
				if count < limit then
					table.insert(split, group)
				else
					break
				end
			end
			prev_index = re2.last_index
		else
			-- TODO: we should advance by one utf16 code unit, or, if the u flag is set,
			-- by one unicode point
			re2.last_index = li + 1
		end
		if re2.last_index > #str then
			local sub = string.sub(str, prev_index, #str)
			table.insert(split, sub)
			break
		end
    end
    return split
end

local function is_digit(c, i)
    local b = string.byte(c, i, i + 1)
    return b >= string.byte('0') and b <= string.byte('9')
end

local function get_substitution(match, str, replacement)
    local result = {}

    local i = 1
    local repl_len = #replacement

    while true do
        local j = string.find(replacement, "$", i, true)
        if j == nil or j + 1 > repl_len then break end
        table.insert(result, string.sub(replacement, i, j - 1))
        local j0 = j
        local c = string.sub(replacement, j + 1, j + 1)
        j = j + 2
        if c == '$' then
            table.insert(result, "$")
        elseif c == '&' then
            table.insert(result, match[0])
        elseif c == '`' then
            table.insert(result, string.sub(str, 1, match.index))
        elseif c == '\'' then
            table.insert(result, string.sub(str, match.index + #match[0]))
        elseif is_digit(c, 1) then
            local k = c
            local kv
            local dig2 = false
            if j <= repl_len and is_digit(replacement, j) then
                k = k .. string.sub(replacement, j, j)
                dig2 = true
            end
            local kv1 = tonumber(k)
            assert(kv1 ~= nil)

            -- This behavior is specified in ES6 and refined in ECMA 2019
            if dig2 and kv1 >= 1 and match[kv1] ~= nil then
                kv = kv1
                j = j + 1
            else
                kv = tonumber(k)
                assert(kv ~= nil)
            end
            if kv >= 1 and match[kv] ~= nil then
                table.insert(result, match[kv])
            else
                table.insert(result, string.sub(replacement, j0, j))
            end
        elseif c == '<' and match.groups ~= nil then
            local k = string.find(replacement, ">", j, true)
            if k == nil then
                table.insert(result, string.sub(replacement, j0, j))
            else
                local name = string.sub(replacement, j, k - 1)
                local capture = match.groups[name]
                assert(capture ~= nil, "invalid capture name: " .. name)
                table.insert(result, capture)
                j = k + 1
            end
        else
            table.insert(result, string.sub(replacement, j0, j))
        end

        i = j
    end
    table.insert(result, string.sub(replacement, i))
    return table.concat(result)
end

function jsregexp.mt.replace_all(re, str, replacement)
    local jstr = jsregexp.to_jsstring(str)

    re.last_index = 1

    local output = {}

    local prev_index = 1
    local cur_index = 1
    while true do
        prev_index = re.last_index
        local match = re:exec(jstr)
        if match == nil then break end
        cur_index = re.last_index

        table.insert(output, string.sub(str, prev_index, match.index - 1))
        if type(replacement) == "function" then
            table.insert(output, replacement(match, str))
        else
            table.insert(output, get_substitution(match, str, replacement))
        end
    end
    table.insert(output, string.sub(str, cur_index))
    return table.concat(output)
end

function jsregexp.mt.replace(re, str, replacement)
	if re.global then
		return jsregexp.mt.replace_all(re, str, replacement)
	end

    local jstr = jsregexp.to_jsstring(str)

    re.last_index = 1

    local output = {}

	local match = re:exec(jstr)
	if match then
		table.insert(output, string.sub(str, 1, match.index - 1))
		if type(replacement) == "function" then
			table.insert(output, replacement(match, str))
		else
			table.insert(output, get_substitution(match, str, replacement))
		end
		table.insert(output, string.sub(str, re.last_index + #match[0] + 1))
	else
		table.insert(output, str)
	end
	return table.concat(output)
end

return jsregexp
