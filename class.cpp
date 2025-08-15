#include "luapython.hpp"

int class_index(lua_State* L) {
    if (!lua_isuserdata(L, -2)) {
        luaL_error(L, "class_index: Attempt to index a %s value", luaL_typename(L, -2));
        return 0;
    }
    const char* key = lua_tostring(L, -1);
    PyObject* obj = *(PyObject**)lua_touserdata(L, -2);
    if (!PyObject_HasAttrString(obj, lua_tostring(L, -1))) {
        luaL_error(L, "class_index: No such attribute %s", key);
        return 0;
    }
    PyObject* attr = PyObject_GetAttrString(obj, key);
    pushLua(L, attr);
    Py_DECREF(attr);
    return 1;
}

int pushClassLua(lua_State* L, PyObject* obj) {
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    Py_INCREF(obj);
    lua_createtable(L, 0, 4);
    lua_pushcfunction(L, class_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}