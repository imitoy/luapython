local core = require "luapython.core"

local loadNative = core.loadNative

-- It is not recommanded to call native method straightly
core.loadNative = nil

function core.load(version, lib_prefix)
    if not(lib_prefix) then
        local check = os.execute("python3-config > /dev/null 2>&1")
        if not(check) then
            lib_prefix = ""
        else
            local handle = io.popen("python3-config --exec-prefix")
            lib_prefix = handle:read()
            handle:close()
        end
    end
    if type(version) == "number" then
        version = tostring(version)
    elseif type(version) == "nil" then
        local command = "python3 --version"
        local check_command = os.execute(command.." > /dev/null 2>&1")
        local check = check_command
        if check_command then
            local handle = io.popen(command)
            local version_info = handle:read()
            local version_extract = string.match(version_info, "%d%.%d")
            if version_extract then
                version = tonumber(version_extract)
            else
                check = false
            end
        end
        if not(check) then
            version = "3"
        end
    elseif type(version) ~= "string" then
        error("core.load: version: string expected, got"..type(version))
    end
    local path = lib_prefix.."/libpython"..version..".so"
    loadNative(path)
end
-- 2. 加载 Lua 编写的 import 包装器
---local importer_generator = require "luapython.import"

-- 3. 从核心库中获取原始的 C 导入函数
--    (我们将在 C 代码中把它命名为 _import)
--local raw_import = core._import

-- 4. 使用包装器创建最终的用户友好型 import 函数
--core.import = importer_generator(raw_import)

-- 5. 从最终的模块表中移除内部函数，保持 API 清洁
--core._import = nil

-- 6. 返回组装完成的模块
return core
