local jsregexp = require "jsregexp"

local lre = {}

function lre.match(re, str)
    local jstr = jsregexp.to_jsstring(str)
    if not re.global then return re:exec(jstr) end
    local matches = {}
    local val

    while true do
        val = re:exec(jstr)
        if val == nil then break end
        table.insert(matches, val[0])
    end
    return matches
end

function lre.match_all(re, str)
    -- must duplicate (according to string.proptype.matchAll spec)
    local re2 = jsregexp.compile(re.source, re.flags .. "g")
    local jstr = jsregexp.to_jsstring(str)
    return function() return re2:exec(jstr) end
end

function lre.match_all_list(re, str)
    local matches = {}
    for match in lre.match_all(re, str) do table.insert(matches, match) end
    return matches
end

function lre.search(re, str)
    -- spec says to start at 1 and restore last_index
    local prev_last_index = re.last_index
    re.last_index = 1
    local match = re:exec(str)
    re.last_index = prev_last_index
    if match == nil then return -1 end
    return match.index
end

function lre.split(re, str, limit)
    if limit == nil then limit = math.huge end
    if limit == 0 then return {} end
    assert(limit >= 0, "limit must be greater than 0")

    local jstr = re.to_jsstring(str)
    local re2 = jsregexp.compile(re.source, re.flags .. "y") -- add sticky

    local count = 0
    local split = {}
    while count < limit do
        local prev_index = re2.last_index
        local match = re2:exec(jstr)
        if match == nil then break end
        local element = string.sub(str, prev_index + 1, match.index - 1)
        if #element > 0 or #match[0] then table.insert(split, element) end
    end
    return split
end

function lre.replace(re, str, replacement)

    local jstr = jsregexp.to_jsstring(str)

    local newstr = {}

    while true do
        local prev_index = re.last_index
        local match = re:exec(jstr)
        if match == nil then break end
        local repl
        if type(replacement) == "function" then
            repl = replacement(match)
        else
            -- todo
            repl = replacement
        end
        table.insert(newstr, string.sub(str, prev_index + 1, match.index - 1))
        table.insert(newstr, repl)
    end
    return table.concat(newstr)
end

return lre
