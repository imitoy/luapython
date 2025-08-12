# luapython

A Lua binding for Python.

**This project is under development...**

## Usage (For now)

1. Install Lua and Python. (latest)

2. Build this project.
```bash
git clone https://github.com/imitoy/luapython.git
cd luapython
make # Requires Python development headers
```
3. Import this library in Lua.
```lua
-- Add cpath
package.cpath=package.cpath..";./?.so" -- Use .dll on Windows

-- Import all
luapython=require "luapython"
-- Then use luapython.import, luapython.set ...

-- Import all to _G or other env
luapython=require "luapython"
luapython:init(_G)
-- Then use _G.import, _G.set ...

-- Import specific function
import=require "luapython.import"
set=require "luapython.set"
-- Then use import, set ...
```

4. Import python modules in Lua.
```lua
numpy=import("numpy") -- Make sure numpy is installed
print(numpy.array({1,2,3}))
```

5. Create Python structure by using `dict`, `set`, `list`, `tuple`.
```lua
json=import"json"
local data = dict{name="Alice", age=18}
print(json.dumps(data))
```
