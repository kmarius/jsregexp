# jsregexp

Provides (a subset of) ECMAScript regular expressions. Uses `libregexp` from Fabrice Bellard's [QuickJS](https://bellard.org/quickjs/).

## Installation

If you intend to use `jsregexp` in neovim you can place

```lua
use_rocks 'jsregexp'
```
    
in the function passed to `packer.startup`.

To install `jsregexp` locally with [luarocks](https://luarocks.org/modules/kmarius/jsregexp), run

```bash
luarocks --local --lua-version 5.1 install jsregexp
```

This will place the compiled module in `$HOME/.luarocks/lib/lua/5.1` so `$HOME/.luarocks/lib/lua/5.1/?.so` needs to be added to `package.cpath`.

Running `make` in this project's root will compile the module `jsregexp.so` and place it in the same directory.

## Usage
This module provides a single function
```lua
jsregexp.compile(regex, flags?)
```
that takes an ECMAScript regular expression as a string and an optional string of flags. Currently only the options `"g"` (match globally) and `"i"` (case insensitive) are recognized.

On success, returns a function that takes a string as its single argument and returns a table containing all matches. On failure, returns `nil` and an error message.

An empty table is returned if the regular expression does not match.

Each `match` consists of a table with the fields

```lua
match.begin_ind  -- begin of the match
match.end_ind    -- end of the match, points at the character following the match (possibly subject to change)
match.groups     -- a table containing the strings of the match group corresponding to the index
```

## Example
```lua
local jsregexp = require("jsregexp")

local regex, err = jsregexp.compile("(\\w)\\w*", "g")
if not regex then
	print(err)
	return
end

local str = "Hello World"
local matches = regex(str)

for i, match in ipairs(matches) do
	print(string.format("match %d: %s", i, str:sub(match.begin_ind, match.end_ind-1)))
	if #match.groups > 0 then
		print("capture groups:")
		for j, group in ipairs(match.groups) do
			print(j, group)
		end
	end
end


```
