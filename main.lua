local a,b = {},{}

local f = function(...)
    local ags = {...}
    print("Function called with arguments:", #ags)
    for i=2,#ags do
        print(ags[i])
    end
    return "string"
end

local index = {
    "__add",
    "__sub",
    "__mul",
    "__div",
    "__mod",
    "__pow",
    "__unm",
    "__idiv",
    "__band",
    "__bor",
    "__bxor",
    "__bnot",
    "__shl",
    "__shr",
    "__concat",
    "__len",
    "__eq",
    "__lt",
    "__le",
    "__index",
    "__newindex",
    "__call",
    "__tostring",
}

for i=1,#index do
    local key = index[i]
    b[key] = f
end

setmetatable(a,b)

local c = a+1
local d = a-2
local e = a*3
local g = a/4
local h = a%5
local i = a^6
local j = -a
local k = a//7
local l = a&8
local m = a|9
local n = a~10   
local o = ~a
local p = a<<11
local q = a>>12
local r = a..13
local s = #a
local t = a==14
local u = a<15
local v = a<=16
local w = a[17]
a[18] = 1
print(a)

print("--------------------------------")

local import = require "luapython"

numpy = import("numpy")

print(numpy,type(numpy))

print(getmetatable(numpy).__index(numpy,"array"))

print(numpy.array({1,2,3}))

print("--------------------------------")



