# luapython

Library of manipulating Python in Lua.

**This project is under development...**

Create an issue if any bug occurred.

> This project is only supported on Linux temporarily.

## Quick start

1. Install Lua and Python. (latest version recommended)

2. Build & install this project.
```bash
git clone https://github.com/imitoy/luapython.git
cd luapython
sudo luarocks install luapython-0.1.1-1.rockspec # require python headers
# when uninstall, replace 'install' with 'remove'
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

6. Create a table to adapt keywords parameters.
```lua
OpenAI = import"openai.OpenAI"
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

## TODO

- [ ] Support for Python version above 3.8
- [ ] Conda support
- [ ] Integrate Python error in Lua