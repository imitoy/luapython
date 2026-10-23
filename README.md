# luapython

Library of manipulating Python in Lua.

**This project is under development...**

Create an issue if any bug occurred.

> This project is only supported on Linux temporarily.

## Requirements

Lua, Python & Luarocks installed on your system.

## Quick start

1. Install Lua and Python. (Latest Lua version recommended)

2. Build & install this project.
```bash
git clone https://github.com/imitoy/luapython.git
cd luapython
sudo luarocks make # require python headers
# when uninstall, run sudo luarocks remove luapython
```
3. Import this library and load with python version in Lua.
```lua
luapython=require "luapython"
luapython.load()
```

4. Import python modules in Lua.
```lua
numpy=luapython.import"numpy" -- Make sure numpy is installed
print(numpy.array({1,2,3}))

math=luapython.import"math"
print(math.tan(90))
```

5. Create Python structure by using `dict`, `set`, `list`, `tuple`.
```lua
json=luapython.import"json"
local data = luapython.dict{name="Alice", age=18}
print(json.dumps(data))
```

6. Create a table to adapt keywords parameters.
```lua
local OpenAI = luapython.import"openai.OpenAI"
local client = OpenAI({api_key="<DeepSeek API Key>", base_url="https://api.deepseek.com"})

local response = client.chat.completions.create({
    model="deepseek-chat",
    messages={
        {role = "system", content = "You are a helpful assistant"},
        {role = "user", content = "Hello"},
    },
    stream=false
})

print(response.choices[0].message.content)
```

7. Append `()` to the Python Iter Object.
```lua
local response = client.chat.completions.create({
    model="deepseek-chat",
    messages={
        {role = "system", content = "You are a helpful assistant"},
        {role = "user", content = "Hello"},
    },
    stream=true
})

for chunk in response() do
    io.write(chunk.choices[0].delta.content)
    io.flush()
end
```

## Use in a virtual env (Conda recommended)
1. Activate virtual env & check `python3-config --exec-prefix`.
```bash
conda activate v_env
python3-config --exec-prefix
```

2. Call the function load().
```lua
local luapython = require"luapython"
local path = luapython.load(3.14, "/path_to_v_env/lib")
-- or luapython.load()                              -- will find libpython automatically
-- or luapython.loadNative("/path_to_libpython.so") -- will load lib directly
```

## Testing

```bash
make test      # builds, stages into build/test-tree, runs test/test.lua
make valgrind  # same suite under valgrind (invalid reads/writes only)
```

The suite runs against Lua 5.1 through 5.5; pass `LUA_INCDIR`, `LUA_VERSION`
and `LUA` to build for a specific interpreter. Objects are ABI-specific, so
switching versions recompiles automatically (`.build-flags` remembers what the
current `.o` files were built against).

## Known limitations

- **Lua 5.1: comparing a Python number with a Lua number does not work.**
  `pynum < 5` raises *"attempt to compare userdata with number"*. Lua 5.1 only
  invokes `__lt`/`__le` when both operands share the very same metamethod, and
  a Lua number has none. Arithmetic works fine; wrap the operand instead
  (`pynum < builtins.int(5)`). Fixed in Lua 5.2 and later.
- **Lua 5.1/5.2 have no integer subtype.** `1` and `1.0` are the same value
  there, so integral numbers are passed to Python as `int` and only fractional
  ones as `float`. On 5.3+ the distinction follows Lua's own.
- **Bitwise operators (`& | ~ << >>`) require Lua 5.3+.** They are not part of
  the language on 5.1/5.2.
- **Keyword arguments are a heuristic.** A trailing table is treated as
  keyword arguments when its keys are mostly strings. Pass containers as
  `luapython.dict{...}` / `luapython.list{...}` to be explicit, and note that a
  table mixing array and named keys is rejected rather than silently truncated.
- **One interpreter per process.** `luapython.load()` starts CPython; a second
  call (or a host that already embedded Python) raises instead of re-entering.
  `luapython.finalize()` releases it, after which every handle obtained earlier
  becomes inert: touching one raises a Lua error instead of dereferencing freed
  memory.

## TODO

- [x] Support for Python version above 3.8
- [x] Support for Lua5.5
- [x] Conda support
- [x] Integrate Python error in Lua
