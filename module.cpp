#include "luapython.hpp"
// #include <iostream>

int module_index(lua_State* L) {
    if (!lua_isstring(L, -1)) {
        luaL_error(L, "module_index: Attempt to index a %s value", luaL_typename(L, -1));
        return 0;
    }
    const char* key = lua_tostring(L, -1);
    PyObject* module = convertPython(L, -2);
    PyObject* value = PyObject_GetAttrString(module, key);
    if (value == NULL) {
        luaL_error(L, "module_index: Attribute %s not found in module %s", key, Py_TYPE(module)->tp_name);
        return 0;
    }
    pushLua(L, value);
    return 1;
}

int module_tostring(lua_State* L) {
    PyObject* module = *(PyObject**)lua_touserdata(L, -1);
    lua_pushstring(L, Py_TYPE(module)->tp_name);
    return 1;
}

int pushModuleLua(lua_State* L, PyObject* module) {
    void* userdata = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)userdata = module;
    Py_INCREF(module);
    lua_createtable(L, 0, 3);
    lua_pushcfunction(L, module_index);
    lua_setfield(L, -2, "__index");
    lua_pushstring(L, Py_TYPE(module)->tp_name);
    lua_setfield(L, -2, "__name");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}