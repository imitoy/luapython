local loader = require("luapython.loader")
local tools = require("luapython.tools")

-- It is not recommended to call the native functions directly, but they are
-- kept for advanced users who want to manage the loading process themselves.
-- loader.loadNative = nil

-- os.execute() returns (true, "exit", 0) on Lua >= 5.2 but a bare exit code
-- on 5.1, where 0 is *truthy*. Normalise both shapes.
local function command_succeeds(command)
    local first, _, code = os.execute(command)
    if code ~= nil then
        return first == true
    end
    return first == 0 or first == true
end

local function capture(command)
    local handle = io.popen(command)
    if not handle then
        return nil
    end
    local output = handle:read("*a")
    handle:close()
    return output
end

-- Return the directory that contains libpython, without a trailing slash.
local function detect_lib_prefix()
    if command_succeeds("python3-config --exec-prefix > /dev/null 2>&1") then
        local prefix = capture("python3-config --exec-prefix")
        if prefix and prefix ~= "" then
            prefix = prefix:gsub("%s+$", "")
            return prefix .. "/lib"
        end
    end
    -- Fall back to the bare library name and let dlopen() search the usual
    -- loader paths (LD_LIBRARY_PATH, ld.so.cache, ...).
    return ""
end

local function detect_version()
    if not command_succeeds("python3 --version > /dev/null 2>&1") then
        return "3"
    end
    local output = capture("python3 --version")
    local version = output and output:match("(%d+%.%d+)")
    return version or "3"
end

-- Candidate file names, most specific first. Many distributions only ship
-- the versioned soname (libpython3.14.so.1.0) unless the -dev package is
-- installed, so a single guess is not enough.
local function lib_candidates(version, prefix)
    local base = (prefix ~= "" and (prefix .. "/") or "") .. "libpython" .. version
    return {
        base .. ".so",
        base .. ".so.1.0",
        base .. ".dylib",
        base .. ".dll",
    }
end

function loader.load(version, lib_prefix)
    if loader.isLoadedNative() then
        return loader.loadedPath
    end

    if lib_prefix == nil then
        lib_prefix = detect_lib_prefix()
    end

    if version == nil then
        version = detect_version()
    elseif type(version) == "number" then
        version = tostring(version)
    elseif type(version) ~= "string" then
        error("luapython.load: version: string expected, got " .. type(version), 2)
    end

    local tried = {}
    local path
    for _, candidate in ipairs(lib_candidates(version, lib_prefix)) do
        local ok, err = pcall(loader.loadNative, candidate)
        if ok then
            path = candidate
            break
        end
        tried[#tried + 1] = tostring(err)
    end

    if not path then
        error("luapython.load: failed to load libpython" .. version
            .. ".\nTried:\n  " .. table.concat(tried, "\n  ")
            .. "\nCall luapython.load(<version>, <lib-dir>) or "
            .. "luapython.loadNative(\"/path/to/libpython.so\") explicitly.", 2)
    end

    local core = require("luapython.core")
    for k, v in pairs(core) do
        loader[k] = v
    end
    -- `init` is releaseToEnv(luapython, env, key); bind the module so that
    -- luapython.init(env) works without passing the module table itself.
    loader.init = function(env, key)
        return tools.releaseToEnv(loader, env, key)
    end
    loader.initialize()
    loader.loadedPath = path
    return path
end

return loader
