local arg = {...}

local env = arg[2] or _G

local key = arg[3]

if type(env) == "string" then
    key = arg
elseif type(env) ~= "table" then
    error("Env table expected")
end

if type(arg[1]) ~= "table" then
    error("First arguement must be a table")
end

if type(arg[1].import) ~= "function" then
    error("Python module expected")
end

if key then
    env[key] = arg[1][key]
else
    for k, v in pairs(arg[1]) do
        if k ~= "init" then
            env[k] = v
        end
    end
end