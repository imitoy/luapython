local func = ({...})[0]

local tools = {}

function tools.astable(obj)
    if type(func.astable) == "function" then
        return func.astable(obj)
    else
        error("luapython.astable: func.astable is not defined")
    end
end

return tools