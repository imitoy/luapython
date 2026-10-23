#include "luapython.h"

int module_index(lua_State* L) {
    if (!isPythonObject(L, -2)) {
        return luaL_error(L, "module_index: attempt to index a %s value", luaL_typename(L, -2));
    }
    int key_type = lua_type(L, -1);
    if (key_type != LUA_TSTRING && key_type != LUA_TNUMBER) {
        return luaL_error(L, "module_index: attribute name must be a string, got %s",
                          luaL_typename(L, -1));
    }
    const char* key = lua_tostring(L, -1);
    PyObject* module = *(PyObject**)lua_touserdata(L, -2);
    if (module == NULL) {
        return luaL_error(L, "module_index: the Python module is no longer valid");
    }
    PyObject* value = PyObject_GetAttrString(module, key);
    if (value == NULL) {
        /* Missing attribute -> nil (import.lua relies on this to fall back
         * from "a.b" to "a" + attribute lookup). Anything else is reported. */
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            lua_pushnil(L);
            return 1;
        }
        return luapython_throw(L, key);
    }
    pushLua(L, value);   /* consumes */
    return 1;
}

int pushModuleLua(lua_State* L, PyObject* obj) {
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_MODULE_NAME)) {
        lua_pushcfunction(L, module_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, python_tostring);
        lua_setfield(L, -2, "__tostring");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}
