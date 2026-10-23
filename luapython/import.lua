-- Wrap the low level importer (`python_import`) so that
--
--   luapython.import "json"            -- a module
--   luapython.import "openai.OpenAI"   -- a class inside a package
--
-- both work. The dotted form first tries a real import, then falls back to
-- importing the root package and walking its attributes.
return function(importer)
    if type(importer) ~= "function" then
        error("luapython.import: function expected, got " .. type(importer), 2)
    end

    return function(module_name)
        if type(module_name) ~= "string" then
            error("luapython.import: string expected, got " .. type(module_name), 2)
        end

        local module = importer(module_name, false)
        if module then
            return module
        end

        local dot = module_name:find("%.")
        if not dot then
            importer(module_name, true)   -- print the original traceback once
            error("luapython.import: module '" .. module_name .. "' not found", 2)
        end

        -- "a.b.c" -> import "a", then walk b, c
        local root_name = module_name:sub(1, dot - 1)
        module = importer(root_name, false)
        if not module then
            importer(root_name, true)
            error("luapython.import: module '" .. root_name .. "' not found", 2)
        end

        for part in module_name:sub(dot + 1):gmatch("[^.]+") do
            local next_value = module[part]
            if next_value == nil then
                error("luapython.import: '" .. module_name .. "' has no member '"
                    .. part .. "'", 2)
            end
            module = next_value
        end

        return module
    end
end
