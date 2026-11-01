local tools = {}

-- Decide whether a Lua table should be converted to a Python dict (true) or
-- to a Python list (false). String keys dominate -> dict, otherwise list.
function tools.shouldConvertToDict(obj)
    if type(obj) ~= "table" then
        error("luapython.tools: table expected, got " .. type(obj), 2)
    end
    local ncount = 0
    local scount = 0
    for key in pairs(obj) do
        if type(key) == "number" then
            ncount = ncount + 1
        elseif type(key) == "string" then
            scount = scount + 1
        end
    end
    return scount >= ncount and scount > 0
end

function tools.releaseToEnv(luapython, env, key)
    if env == nil and key == nil then
        env = _G
    elseif type(env) == "string" and key == nil then
        key = env
        env = _G
    elseif type(env) ~= "table" then
        error("luapython.init: env table expected, got " .. type(env), 2)
    end

    if key ~= nil and type(key) ~= "string" then
        error("luapython.init: key must be a string", 2)
    end

    if type(luapython) ~= "table" then
        error("luapython.init: first argument must be a table", 2)
    end

    if key then
        env[key] = luapython[key]
    else
        for k, v in pairs(luapython) do
            if k ~= "init" then
                env[k] = v
            end
        end
    end
end

-- `{ ... }` cannot remember trailing nils: `#{1, nil}` is 1 and `#{nil}` is 0,
-- so `f(1, nil)` used to become `f(1)` and `f(nil)` became `f()` (which made
-- `json.dumps(nil)` fail with "missing required argument 'obj'"). Record the
-- real argument count alongside the values; the native side already takes the
-- count as its own parameter, so nothing else has to change.
local function pack(...)
    return { n = select("#", ...), ... }
end

-- Wrap a raw python_function so that a trailing table is treated as keyword
-- arguments. Note this is a heuristic: a callable that expects a dict as its
-- *last positional* argument must receive it wrapped with luapython.dict{}.
function tools.getPythonAdaptFunction(python_function)
    if type(python_function) ~= "function" then
        error("luapython.tools: function expected, got " .. type(python_function), 2)
    end

    return function(self, ...)
        local args = pack(...)
        local n = args.n
        local last = args[n]
        local arg_dict = {}
        -- An empty table is not a keyword-argument list: f({}) must pass an
        -- empty container, not vanish into f().
        if type(last) == "table" and next(last) ~= nil and tools.shouldConvertToDict(last) then
            arg_dict = last
            args[n] = nil
            n = n - 1
        end
        return python_function(self, args, arg_dict, n)
    end
end

function tools.getIterFunction(iter, get)
    if type(iter) ~= "function" then
        error("luapython.tools: first argument is not a function", 2)
    end
    if type(get) ~= "function" then
        error("luapython.tools: second argument is not a function", 2)
    end

    local function iter_func(state)
        return iter, get(state), nil
    end

    return iter_func
end

return tools
