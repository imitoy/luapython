# luapython

A Lua binding for Python.

**This project is under development...**

## Usage (For now)

1. Install Lua and Python. (latest)

2. Build this project.
```bash
git clone https://github.com/imitoy/luapython.git
cd luapython
make # Requires Lua and Python headers
sudo make install
# when uninstall, just replace with `uninstall`
```
3. Import this library in Lua.
```lua
-- Import all
luapython=require "luapython"
-- Then use luapython.import, luapython.dict ...

-- Import all to _G or other env
luapython=require "luapython"
luapython:init(_G)
-- Then use _G.import, _G.dict ...

-- Import specific function
import=require "luapython.import"
-- Then use import
```

4. Import python modules in Lua.
```lua
numpy=import"numpy" -- Make sure numpy is installed
print(numpy.array({1,2,3}))
math=import"math"
print(math.tan(90))
```

5. Create Python structure by using `dict`, `set`, `list`, `tuple`.
```lua
json=import"json"
local data = dict{name="Alice", age=18}
print(json.dumps(data))
```
