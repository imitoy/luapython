-- 1. 加载 C 核心库
local core = require "luapython.core"

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
