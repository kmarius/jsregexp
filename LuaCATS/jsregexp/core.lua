---@meta

local jsregexp = {}

---Compile a regular expression. Throws if an error occurs.
---```lua
---    local re = jsregexp.compile("\\w+", "g")
---```
---@param re string
---@param flags? string
---@return JSRegExp.RegExp RegExp
function jsregexp.compile(re, flags) end

---Safely compile a regular expression.
---```lua
---    local re, err = jsregexp.compile_safe("\\w+", "g")
---    if not re then
---      print(err)
---    end
---```
---@param re string
---@param flags? string
---@return JSRegExp.RegExp? RegExp
---@return string? Error
function jsregexp.compile_safe(re, flags) end

---Convert a lua utf8 lua string to a utf16 js string. For internal use.
---@param str string
---@return JSRegExp.JSString
function jsregexp.to_jsstring(str) end

---UTF-16 representation of a string. For internal use.
---@class JSRegExp.JSString

---A compiled JavaScript regular expression object.
---@class JSRegExp.RegExp
---@field last_index integer the position at wchich the next match will be searched in re:exec or re:test (see notes below)
---@field source string the regexp string
---@field flags string a string representing the active flags
---@field dot_all boolean is the dod_all flag set?
---@field global boolean is the global flag set?
---@field has_indices boolean is the indices flag set?
---@field ignore_case boolean is the ignore_case flag set?
---@field multiline boolean is the multiline flag set?
---@field sticky boolean is the sticky flag set?
---@field unicode boolean is the unicode flag set?
local re = {}

---Execute the regular expression against a string.
---```lua
---    local re = jsregexp.compile("\\w+", "g")
---    local match = re:exec("Hello World")
---    if match then
---      print(match) -- Hello
---    end
---```
---@param string string|JSRegExp.JSString
---@return JSRegExp.Match?
function re:exec(string) end

---Test the regular expression against a string.
---```lua
---    local re = jsregexp.compile("\\w+", "g")
---    if re:test("Hello World") then
---      print("Matched!")
---    end
---```
---@param string string|JSRegExp.JSString
---@return boolean
function re:test(string) end

---Returns a list of all matches or nil if no match.
---```lua
---    local re = jsregexp.compile("\\w+", "g")
---    local matches = re:match("Hello World")
---    if matches then
---      for _, match in ipairs(matches) do
---        print(match)
---      end
---    end
---```
---@param string string
---@return JSRegExp.Match[]?
function re:match(string) end

---Returns a closure that repeatedly calls re:exec, to be used in for-loops.
---```lua
---    local re = jsregexp.compile("\\w+", "g")
---    for match in re:match_all("Hello World") do
---      print(match)
---    end
---```
---@param string string
---@return fun():JSRegExp.Match
function re:match_all(string) end

---Returns a list of all matches.
---```lua
---    local re = jsregexp.compile("\\w+", "g")
---    local matches = re:match("Hello World")
---    for _, match in ipairs(matches) do
---      print(match)
---    end
---```
---@param string string
---@return JSRegExp.Match[]
function re:match_all_list(string) end

---Returns the 1-based index of the first match of re in str, or -1 if no match
---```lua
---    local re = jsregexp.compile("Wo\\w+")
---    local idx = re:match("Hello World")
---    if idx > 0 then
---      print("Matched at index " .. idx)
---    end
---```
---@param string string
---@return integer
function re:search(string) end

---Splits str at re, at most `limit` times
---```lua
---    local re = jsregexp.compile("\\s+")
---    local res = re:split("Hello World")
---    for _, str in ipairs(res) do
---      print(str)
---    end
---```
---@param string string
---@param limit? integer
---@return string[]
function re:split(string, limit) end

---Relplace the first match (all matches, if global) of re in str by replacement.
---```lua
---    local re = jsregexp.compile("\\w+")
---    local res = re:replace("Hello World", "Hey")
---    print(res) -- Hey World
---    local res2 = re:replace("Hello World", function(match, str)
---      if match[0] == "Hello" then
---        return "Hey"
---      elseif match[0] == "World" then
---        return "Joe"
---      else
---        return ""
---      end
---    end)
---    print(res2) -- Hey World
---```
---@param string string
---@param replacement (fun(match: JSRegExp.Match, str: string):string)|string
---@return string
function re:replace(string, replacement) end

---Relplace all occurances of re in str by replacement.
---```lua
---    local re = jsregexp.compile("\\w+", "g")
---    local res = re:replace_all("Hello World", "Hey")
---    print(res) -- Hey Hey
---    local res2 = re:replace_all("Hello World", function(match, str)
---      if match[0] == "Hello" then
---        return "Hey"
---      elseif match[0] == "World" then
---        return "Joe"
---      else
---        return ""
---      end
---    end)
---    print(res2) -- Hey Joe
---```
---@param string string
---@param replacement (fun(match: JSRegExp.Match, str: string):string)|string
---@return string
function re:replace_all(string, replacement) end

---@class JSRegExp.Match : table
---@field input string The input string
---@field capture_count integer The number of capture groups
---@field index integer The start of the capture (1-based)
---@field groups table<string,string> Table of the named groups and their content
---@field indices JSRegExp.MatchIndices Array of begin/end indices of numbered match groups (if `"d"` flag is set)

---@class JSRegExp.MatchIndices : table
---@field groups table Table of named groups and their begin/end indices (if `"d"` flag is set)

return jsregexp
