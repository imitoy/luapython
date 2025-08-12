# LuaPython

**This project is developing...**

LuaPython is a lua binding for python.

## Usage (For now)

1. Install Lua and Python.

2. Build this project. ```bash
$ git clone https://github.com/imitoy/luapython.git
$ cd luapython
$ make
```
3. Import this library in Lua. ```lua
luapython=require "luapython"
luapython:init()
``` Or, require the `import` function directly. ```lua
import=require "luapython.import"
```

4. Import python modules in Lua. ```lua
import("numpy") --make sure numpy is installed
print(numpy.array({1,2,3}))
```