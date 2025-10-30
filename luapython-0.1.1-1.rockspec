package = "luapython"
version = "0.1.1-1"
source = {
   url = "git+https://github.com/imitoy/luapython.git"
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

rockspec_format = "3.0"
dependencies = {
   "lua >= 5.1, <= 5.4",
}
external_dependencies = {
   PYTHON = {
      header = "python3.13/Python.h"
   }
}
build = {
   type = "builtin",
   modules = {
      ["luapython"] = "luapython/init.lua",
      ["luapython.import"] = "luapython/import.lua",
      ["luapython.tools"] = "luapython/internal/tools.lua",
      ["luapython.core"] = {
         sources = {
            "luapython/luapython.c",
            "luapython/number.c",
            "luapython/string.c",
            "luapython/set.c",
            "luapython/dict.c",
            "luapython/list.c",
            "luapython/tuple.c",
            "luapython/module.c",
            "luapython/function.c",
            "luapython/class.c",
            "luapython/iter.c",
            "luapython/tools.c",
         },
         libraries = {
            "python3.13",
            "dl",
            "m"
         },
      },
   },
}
