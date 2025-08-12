

luapython:init()

local json=import("json")

--local data = dict{name="test",age=18}
--print(json.dumps(data))
for k, v in pairs(getmetatable(json)) do
    print(k,v)
end
print(type(json))
local r = json.dumps
for k, v in pairs(getmetatable(json)) do
    print(k,v)
end
for k, v in pairs(getmetatable(r)) do
    print(k,v)
end
print(r==json)
print(type(json),getmetatable(json),getmetatable(json).__index)
print(json.dumps,getmetatable(json))
return r