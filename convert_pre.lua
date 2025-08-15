local args = { ... }

local arg = args[1]

local ncount = 0
local scount = 0

if type(arg) ~= "table" then
    error("convert_pre.lua: table expected, got " .. type(arg))
end

for i, v in pairs(arg) do
    if type(i) == "number" then
        ncount = ncount + 1
    elseif type(i) == "string" then
        scount = scount + 1
    end
end

return scount > ncount
