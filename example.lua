package.cpath=package.cpath..";./?.so"
luapython = require("luapython")

luapython:init()

local json=import("json")

local data = dict{name="test",age=18}
print(json.dumps(data))