local arg = {...}
local iter = arg[1]
local get = arg[2]

if type(iter) ~= "function" then
    error("iter.lua: first argument is not a function")
end

if type(get) ~= "function" then
    error("iter.lua: second argument is not a function")
end

local function iter_func(array)
    return iter, get(array), nil
end

return iter_func