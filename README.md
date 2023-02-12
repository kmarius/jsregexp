# jsregexp

Provides ECMAScript regular expressions for Lua 5.1, 5.2, 5.3, 5.4 and LuaJit. Uses `libregexp` from Fabrice Bellard's [QuickJS](https://bellard.org/quickjs/).

## Installation

To install `jsregexp` globally with [luarocks](https://luarocks.org/modules/kmarius/jsregexp),
run
```bash
sudo luarocks install jsregexp
```
To install `jsregexp` for a different lua version (in this case Lua5.1 or LuaJit), run
```bash
sudo luarocks --lua-version 5.1 install jsregexp
```

To install `jsregexp` locally for your user, run

```bash
luarocks --local --lua-version 5.1 install jsregexp
```

This will place the compiled module in `$HOME/.luarocks/lib/lua/5.1` so `$HOME/.luarocks/lib/lua/5.1/?.so` needs to be added to `package.cpath`.

Simply running `make` in this project's root will compile the module `jsregexp.so` (tested on linux only).

## Usage
This module provides two functions
```lua
jsregexp.compile(regex, flags?)
jsregexp.compile_safe(regex, flags?)
```
that take an ECMAScript regular expression as a string and an optional string of flags, most notably

- `"i"`: case insensitive search
- `"g"`: match globally
- `"n"`: enables named groups (not present in JavaScript, needs to be enabled manually if needed)
- `"u"`: utf-16 support if detected in the pattern string (**implicity set**)

The complete list of flags can be found in the [JavaScript reference](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp/RegExp#parameters).

On success, `compile` and `compile_safe` return a RegExp object. On failure, `compile` throws an error while `compile_save` returns `nil` and an error message.

### RegExp object

Each RegExp object `re` has the following fields
```lua
re.last_index   -- the position at wchich the next match will be searched in re:exec or re:test (see notes below)
re.source       -- the regexp string
re.flags        -- a string representing the active flags
re.dot_all      -- is the dod_all flag set?
re.global       -- is the global flag set?
re.ignore_case  -- is the ignore_case flag set?
re.multiline    -- is the multiline flag set?
re.sticky       -- is the sticky flag set?
re.unicode      -- is the unicode flag set?
```

The RegExp object `re` has the following methods corresponding to JavaScript regular expressions:
```lua
re:exec(str)                      -- returns the next match of re in str (see notes below)
re:test(str)                      -- returns true if the regex matches str (see notes below)
re:match(str)                     -- returns a list of all matches or nil if no match
re:match_all(str)                 -- returns a closure that repeatedly calls re:exec, to be used in for-loops
re:match_all_list(str)            -- returns a list of all matches
re:search(str)                    -- returns the 1-based index of the first match of re in str, or -1 if no match
re:split(str, limit?)             -- splits str at re, at most limit times
re:replace(str, replacement)      -- relplace the first match of re in str by replacement (all, if global)
re:replace_all(str, replacement)  -- relplace each match of re in str by replacement
```
For the documentation of the behaviour of each of these functions, see the [JavaScript reference](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp).

**Note:** Each regexp object has a field `last_index` which denotes the position at which the next call to `exec` and `test` searches for the next match.
Afterwards `last_index` is changed accordingly. If you need to use these methods, you should reset `last_index` to 1.

**Note:** Because the regexp engine used works with UTF16 instead of UTF8, the input string is converted to UTF16 if necessary. Calling `exec` or `test` on
non-Ascii strings repeatedly could potentially introduce a large overhead. This conversion only needs to be done once for the `match*` methods, you probably want to use those instead.


### Match object

A match object `m` returned by `exec` and the `match*` functions has the following fields:
```lua
m[0]             -- the full match
m[i]             -- match group i
m.input          -- the input string
m.capture_count  -- number of capture groups
m.index          -- start of the capture (1-based)
m.groups         -- table of the named groups and their content
```

## Example
```lua
local jsregexp = require("jsregexp")

local re, err = jsregexp.compile_safe("(\\w)\\w*", "g")
if not re then
	print(err)
	return
end

local str = "Hello World"

for match in re:match_all(str) do
	print(match)
	for j, group in ipairs(match) do
		print(j, group)
	end
end
```
