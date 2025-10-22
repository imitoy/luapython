package = "luapython"
version = "0.1.0"

source = {
  url = "git://github.com/imitoy/luapython"
}

description = {
  summary = "Use Python from Lua (C extension + helper Lua modules)",
  detailed = [[
luapython lets you import and call Python from Lua. It provides a compiled
C module and Lua helpers (import, python_init, python_function, tools, iter, etc.).
  ]],
  homepage = "https://github.com/imitoy/luapython",
  license = "MIT"
}

supported_platforms = { "linux" }

-- Require Lua; module supports Lua 5.1–5.4 and LuaJIT (configure.lua auto-detects)
dependencies = {
  "lua >= 5.1"
}

-- Ensure Python headers are available (Python.h)
external_dependencies = {
  PYTHON = {
    -- header = "Python.h"
  }
}

build = {
  type = "make",

  -- Generate Makefile with the right compiler/Lua version, then build.
  build_command = [[
    ${LUA:-lua} configure.lua && \
    ${MAKE:-make}
  ]],

  -- Install into LuaRocks tree instead of system prefix
  install_command = [[
    ${MAKE:-make} install
  ]]
}
