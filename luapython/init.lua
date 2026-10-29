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

-- Directories that may contain libpython, in the order we try them. The empty
-- string stands for "no directory": dlopen() then searches LD_LIBRARY_PATH and
-- ld.so.cache, which is the last resort.
--
-- Several layout quirks have to be covered here:
--  * Debian/Ubuntu keep the library in a multiarch directory
--    (/usr/lib/x86_64-linux-gnu), which `python3-config --exec-prefix` does
--    not report -- only sysconfig knows it.
--  * A relocated or vendored interpreter (conda, pyenv, a frozen runtime) can
--    report a build-time LIBDIR like "/install" that does not exist at all, so
--    the exec-prefix guess must still be tried afterwards.
local function detect_lib_dirs()
    local dirs = {}
    local function add(dir)
        if not dir then
            return
        end
        dir = dir:gsub("%s+$", "")
        if dir == "" then
            return
        end
        for _, existing in ipairs(dirs) do
            if existing == dir then
                return
            end
        end
        dirs[#dirs + 1] = dir
    end

    add(capture([[python3 -c "import sysconfig;print(sysconfig.get_config_var('LIBDIR') or '')" 2>/dev/null]]))
    if command_succeeds("python3-config --exec-prefix > /dev/null 2>&1") then
        local prefix = capture("python3-config --exec-prefix")
        if prefix then
            add(prefix:gsub("%s+$", "") .. "/lib")
        end
    end
    dirs[#dirs + 1] = ""
    return dirs
end

local function detect_version()
    if not command_succeeds("python3 --version > /dev/null 2>&1") then
        return "3"
    end
    local output = capture("python3 --version")
    local version = output and output:match("(%d+%.%d+)")
    return version or "3"
end

-- Candidate file names for one directory. Many distributions only ship the
-- versioned soname (libpython3.14.so.1.0) unless the -dev package is
-- installed, so a single guess is not enough. An empty `prefix` means "bare
-- file name", i.e. let the dynamic loader resolve it.
local function lib_candidates(version, prefix)
    local base = "libpython" .. version
    if prefix == "" then
        -- Bare names: let the dynamic loader search its usual paths. Keep the
        -- macOS/Windows extensions the explicit-directory list has, otherwise
        -- those platforms lose the last-resort lookup.
        return {
            base .. ".so",
            base .. ".so.1.0",
            base .. ".dylib",
            base .. ".dll",
        }
    end
    return {
        prefix .. "/" .. base .. ".so",
        prefix .. "/" .. base .. ".so.1.0",
        prefix .. "/" .. base .. ".dylib",
        prefix .. "/" .. base .. ".dll",
    }
end

function loader.load(version, lib_prefix)
    -- After finalize() every handle we handed out is dangling, so the engine
    -- must not be started a second time. Check this before the isLoadedNative()
    -- shortcut below: libpython stays dlopen()ed, which would otherwise make
    -- load() report success while the interpreter is dead.
    if loader.isFinalized and loader.isFinalized() then
        error("luapython.load: the Python interpreter has been finalized and "
            .. "cannot be restarted in this process", 2)
    end
    if loader.isLoadedNative() then
        return loader.loadedPath
    end

    -- An explicit directory is taken literally (a virtualenv that ships no
    -- libpython must fail instead of silently loading the system one); only
    -- auto-detection gets the extra guesses.
    local dirs = (lib_prefix ~= nil) and { lib_prefix } or detect_lib_dirs()

    if version == nil then
        version = detect_version()
    elseif type(version) == "number" then
        version = tostring(version)
    elseif type(version) ~= "string" then
        error("luapython.load: version: string expected, got " .. type(version), 2)
    end

    local tried = {}
    local path
    for _, dir in ipairs(dirs) do
        for _, candidate in ipairs(lib_candidates(version, dir)) do
            local ok, err = pcall(loader.loadNative, candidate)
            if ok then
                path = candidate
                break
            end
            tried[#tried + 1] = tostring(err)
        end
        if path then
            break
        end
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
