local args = {...}

for _, arg in ipairs(args) do
    if arg == "--help" or arg == "-h" then
        print("Usage: lua configure.lua [options]")
        print("Options:")
        print("  --help, -h                 Show this help message")
        print("  --version=<Lua Version>    Show version information")
        os.exit(0)
    end
end

local function getPythonVersion()
    local handle = io.popen("python3 --version")
    local result = handle:read("*a")
    handle:close()
    local version = result:match("Python (%d+%.%d+)%.%d+")
    return version
end

local function getLuaVersion()
    return _VERSION:match("Lua (%d+%.%d+)")
end