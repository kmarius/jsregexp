local jsreg = require("jsregexp");

local s = "this is a string"
local r = "(\\w+)\\s*(\\w+)"

print(string.format('matching "%s" against "%s"', r, s))
for i, e in pairs(jsreg.gmatch(s, r)) do
	print(string.format("match: %s, pos: %d, len: %d", e[0], e.position, e.length))

	for j = 1,#e do
		print(string.format("group %d: %s", j, e[j]))
	end
end
