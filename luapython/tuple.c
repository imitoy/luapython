#include "luapython.h"

/* Lua 5.1 hands `__len` two arguments (the object and a nil), 5.2+ hands over
 * only one. Address the object as argument #1 so both work. */
int tuple_len(lua_State* L) {
    if (!isPythonTuple(L, 1)) {
        return luaL_error(L, "tuple_len: attempt to get length of %s", luaL_typename(L, 1));
    }
    PyObject* tuple = *(PyObject**)lua_touserdata(L, 1);
    lua_pushinteger(L, (lua_Integer)PyTuple_Size(tuple));
    return 1;
}

int tuple_index(lua_State* L) {
    if (!isPythonTuple(L, -2)) {
        return luaL_error(L, "tuple_index: attempt to index %s", luaL_typename(L, -2));
    }
    lua_Integer idx = luaL_checkinteger(L, -1);
    PyObject* tuple = *(PyObject**)lua_touserdata(L, -2);
    Py_ssize_t len = PyTuple_Size(tuple);

    /* Same 1-based mapping as list: t[1] -> py[0], t[-1] -> last element. */
    Py_ssize_t py_idx;
    if (idx > 0) {
        py_idx = (Py_ssize_t)idx - 1;
    } else if (idx < 0) {
        py_idx = len + (Py_ssize_t)idx;
    } else {
        py_idx = -1;
    }
    if (py_idx < 0 || py_idx >= len) {
        lua_pushnil(L);
        return 1;
    }
    PyObject* value = PyTuple_GetItem(tuple, py_idx);   /* borrowed */
    Py_XINCREF(value);
    pushLua(L, value);                                  /* consumes */
    return 1;
}

int pushTupleLua(lua_State* L, PyObject* obj) {
    if (!PyTuple_Check(obj)) {
        Py_XDECREF(obj);
        return luaL_error(L, "pushTupleLua: not a tuple");
    }
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_TUPLE_NAME)) {
        lua_pushcfunction(L, tuple_len);
        lua_setfield(L, -2, "__len");
        lua_pushcfunction(L, tuple_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, python_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertTuplePython(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TTABLE) {
        PyObject* list = convertListPython(L, index);
        if (list == NULL) {
            return NULL;
        }
        PyObject* tuple = PyList_AsTuple(list);
        Py_DECREF(list);
        return tuple;
    }
    if (isPythonTuple(L, index)) {
        PyObject* tuple = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(tuple);
        return tuple;
    }
    luaL_error(L, "convertTuplePython: expected a table or tuple at index %d, got %s",
               index, luaL_typename(L, index));
    return NULL;
}
