local func = ({ ... })[1]

if type(func) ~= "function" then
    error("python_function.lua: function expected, got " .. type(func))
end

return function(...)
    local self = select(1, ...)
    local args = { ... }
    table.remove(args, 1)
    local n = #args
    return func(self, table.unpack(args, 1, n), n)
end