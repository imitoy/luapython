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

print(response.choices[1].message.content)
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
    io.write(chunk.choices[1].delta.content)
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
  table mixing array and named keys is rejected rather than silently truncated
  -- but only on the ordinary table-conversion path. The keyword-argument path
  is the exception: a trailing table read as keyword arguments drops its array
  part silently, so `builtins.dict{1, a="x"}` yields `{'a': 'x'}` and the
  numeric entry is lost.
- **Table conversion is recursive and depth-limited.** A self-referential
  table cannot be represented in Python, and nesting deeper than 200 levels is
  rejected; both raise a Lua error instead of overflowing the C stack.
- **Indexing: containers have their own rules, generic objects merge
  `getattr` and `__getitem__`.** `luapython.list` and `luapython.tuple` use
  Lua's 1-based subscripts (`l[1]` is the first element, `l[-1]` the last) and
  do not expose Python attributes, so `luapython.list{1}.append` is not
  available; `luapython.set` is not subscriptable at all. `luapython.dict`
  looks the key up verbatim in Python (`zd[0]` is the entry whose key is `0`,
  not the first entry) and only falls back to attribute lookup when the key is
  absent, which is what makes `d.get(...)` work. For every *other* Python
  object Lua has one indexing operator, so `obj.name` and `obj[key]` cannot be
  told apart: an attribute is tried first, then `obj[key]`, and either way an
  absent key yields `nil`. Numeric subscripts on that path are 1-based like
  `luapython.list` (`o[1]` is the first element, `o[-1]` the last), so
  `range(10)[1]` is `0`. That is what makes `numpy.array{...}[1]` and
  `os.environ["PATH"]` work. Assignment on that path is item-first, the
  intuitive reading of `o[k] = v`: numeric subscripts go to `__setitem__` with
  the very same 1-based mapping (`ba[1] = 65` writes the element `ba[1]` reads
  back; `o[0] = v` is an error, because a Lua sequence has no element 0), and
  string keys go to `__setitem__` first, falling back to `setattr` only when
  the object has no item-assignment protocol at all (the failure is an
  `AttributeError`, or a `TypeError` saying the object is not subscriptable /
  does not support item assignment). Any other error is item semantics
  (unhashable key, wrong value type, ...) and surfaces unchanged instead of
  being masked by a `setattr` attempt. Hence `os.environ["X"] = "y"` really
  sets the environment variable -- even when `X` collides with a mapping
  method such as `get`, which an earlier getattr-probe routing used to
  overwrite with the value -- while `obj.attr = v` keeps behaving like Python.
  Objects that support both items and attributes therefore have a deliberate
  asymmetry: a write lands in the item (Python's `o[k] = v`) while the
  matching read prefers the attribute, so `os.environ["get"] = "x"` sets the
  variable but `os.environ["get"]` still reads the `get` method; use Python
  `setattr` when the attribute is what you want. If both the item write and
  the `setattr` fallback fail, the error reports the item-side exception and
  the `setattr` failure together.
  `luapython.dict` also supports
  `for k, v in pairs(d)` on Lua 5.2+; a `None` key is skipped, because `nil`
  would end the generic `for` and hide every remaining entry.
- **Equality is Python equality.** Containers, numbers, modules, functions and
  generic objects compare with Python `==` instead of Lua's reference
  comparison, so two handles to the same list (or to the same imported module)
  compare equal, and comparing one of them with an unrelated value is `false`
  rather than an error.
- **One interpreter per process.** `luapython.load()` starts CPython; a second
  call (or a host that already embedded Python) raises instead of re-entering.
  `luapython.finalize()` releases it, after which every handle obtained earlier
  becomes inert: touching one raises a Lua error instead of dereferencing freed
  memory. The interpreter cannot be started again afterwards -- `isFinalized()`
  reports it, and `load()` / `initialize()` raise instead of pretending to
  succeed.
- **Python's buffered output is flushed at exit.** If the script never calls
  `finalize()`, the library finalizes the interpreter from `atexit` so that
  `print()` output is not lost when stdout is redirected to a file or a pipe.

## TODO

- [x] Support for Python version above 3.8
- [x] Support for Lua5.5
- [x] Conda support
- [x] Integrate Python error in Lua
