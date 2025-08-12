local args = {...}

local arg = args[1]

local ncount = 0
local scount = 0

for i,v in pairs(arg) do
    if type(i) == "number" then
        ncount = ncount + 1
    elseif type(i) == "string" then
        scount = scount + 1
    end
end

return scount > ncount