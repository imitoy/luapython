#include <python3.13/Python.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef PREFIX
#define PREFIX "/usr"
#endif

#define PYTHON_OBJECT_NAME "python_object"

int luaopen_luapython(lua_State* L);

int python_tostring(lua_State* L);
int python_gc(lua_State* L);

bool isPythonObject(lua_State* L, int index);

int luapython_astable(lua_State* L);

int pushNumberLua(lua_State* L, PyObject* number);
int pushStringLua(lua_State* L, PyObject* string);
int pushSetLua(lua_State* L, PyObject* set);
int pushDictLua(lua_State* L, PyObject* dict);
int pushTupleLua(lua_State* L, PyObject* tuple);
int pushListLua(lua_State* L, PyObject* list);
int pushFunctionLua(lua_State* L, PyObject* function);
int pushModuleLua(lua_State* L, PyObject* module);
int pushClassLua(lua_State* L, PyObject* obj);
int pushIterLua(lua_State* L, PyObject* iter);

int pushLua(lua_State* L, PyObject* obj);

PyObject* convertNumberPython(lua_State* L, int index);
PyObject* convertBooleanPython(lua_State* L, int index);
PyObject* convertStringPython(lua_State* L, int index);
PyObject* convertSetPython(lua_State* L, int index);
PyObject* convertDictPython(lua_State* L, int index);
PyObject* convertTuplePython(lua_State* L, int index);
PyObject* convertListPython(lua_State* L, int index);
PyObject* convertFunctionPython(lua_State* L, int index);
PyObject* convertModulePython(lua_State* L, int index);

PyObject* convertPython(lua_State* L, int index);