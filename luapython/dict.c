#include "luapython.h"

#define isPythonDict(L, index) (isPythonObject(L, index) && PyDict_Check(*(PyObject**)lua_touserdata(L, index)))

/* Fetch operand `index` as a Python dict (converting a Lua table if needed).
 * Returns a new reference or NULL. */
static PyObject* dict_operand(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TTABLE) {
        return convertPython(L, index);
    }
    if (isPythonDict(L, index)) {
        PyObject* dict = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(dict);
        return dict;
    }
    return NULL;
}

/* Lua 5.1 hands `__len` two arguments (the object and a nil), 5.2+ hands over
 * only one. Address the object as argument #1 so both work. */
int dict_len(lua_State* L) {
    if (lua_type(L, 1) == LUA_TTABLE) {
#if LUA_VERSION_NUM >= 502
        lua_len(L, 1);
#else
        lua_pushnumber(L, lua_objlen(L, 1));
#endif
        return 1;
    }
    if (!isPythonDict(L, 1)) {
        return luaL_error(L, "dict_len: attempt to get length of %s", luaL_typename(L, 1));
    }
    PyObject* dict = *(PyObject**)lua_touserdata(L, 1);
    lua_pushinteger(L, (lua_Integer)PyDict_Size(dict));
    return 1;
}

int dict_index(lua_State* L) {
    if (!isPythonDict(L, -2)) {
        return luaL_error(L, "dict_index: attempt to index %s", luaL_typename(L, -2));
    }
    PyObject* dict = *(PyObject**)lua_touserdata(L, -2);
    PyObject* key = convertPython(L, -1);
    if (key == NULL) {
        return luapython_throw(L, "dict_index: cannot convert key");
    }
    PyObject* value = PyDict_GetItemWithError(dict, key);   /* borrowed */
    Py_DECREF(key);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return luapython_throw(L, "dict_index");
        }
        lua_pushnil(L);
        return 1;
    }
    Py_XINCREF(value);
    pushLua(L, value);   /* consumes */
    return 1;
}

int dict_newindex(lua_State* L) {
    if (!isPythonDict(L, -3)) {
        return luaL_error(L, "dict_newindex: attempt to assign to %s", luaL_typename(L, -3));
    }
    PyObject* dict = *(PyObject**)lua_touserdata(L, -3);
    PyObject* key = convertPython(L, -2);
    PyObject* value = convertPython(L, -1);
    if (key == NULL || value == NULL) {
        Py_XDECREF(key);
        Py_XDECREF(value);
        return luapython_throw(L, "dict_newindex: cannot convert key or value");
    }
    int rc = PyDict_SetItem(dict, key, value);   /* does NOT steal */
    Py_DECREF(key);
    Py_DECREF(value);
    if (rc != 0) {
        return luapython_throw(L, "dict_newindex");
    }
    return 0;
}

/* d1 + d2 : merge, values of d2 win */
int dict_add(lua_State* L) {
    PyObject* left = dict_operand(L, -2);
    PyObject* right = dict_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "dict_add: attempt to merge %s and %s",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }
    PyObject* result = PyDict_Copy(left);
    if (result == NULL) {
        Py_DECREF(left);
        Py_DECREF(right);
        return luapython_throw(L, "dict_add");
    }
    int rc = PyDict_Update(result, right);
    Py_DECREF(left);
    Py_DECREF(right);
    if (rc != 0) {
        Py_DECREF(result);
        return luapython_throw(L, "dict_add");
    }
    pushDictLua(L, result);   /* consumes */
    return 1;
}

/* d1 * d2 : intersection (keys present in both, values from d1) */
int dict_mul(lua_State* L) {
    PyObject* left = dict_operand(L, -2);
    PyObject* right = dict_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "dict_mul: attempt to intersect %s and %s",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }
    PyObject* result = PyDict_New();
    if (result == NULL) {
        Py_DECREF(left);
        Py_DECREF(right);
        return luapython_throw(L, "dict_mul");
    }
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(left, &pos, &key, &value)) {
        int contains = PyDict_Contains(right, key);
        if (contains < 0 || (contains && PyDict_SetItem(result, key, value) != 0)) {
            Py_DECREF(left);
            Py_DECREF(right);
            Py_DECREF(result);
            return luapython_throw(L, "dict_mul");
        }
    }
    Py_DECREF(left);
    Py_DECREF(right);
    pushDictLua(L, result);   /* consumes */
    return 1;
}

/* d1 - d2 : difference (keys of d1 not present in d2) */
int dict_sub(lua_State* L) {
    PyObject* left = dict_operand(L, -2);
    PyObject* right = dict_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "dict_sub: attempt to subtract %s from %s",
                          luaL_typename(L, -1), luaL_typename(L, -2));
    }
    PyObject* result = PyDict_New();
    if (result == NULL) {
        Py_DECREF(left);
        Py_DECREF(right);
        return luapython_throw(L, "dict_sub");
    }
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(left, &pos, &key, &value)) {
        int contains = PyDict_Contains(right, key);
        if (contains < 0 || (!contains && PyDict_SetItem(result, key, value) != 0)) {
            Py_DECREF(left);
            Py_DECREF(right);
            Py_DECREF(result);
            return luapython_throw(L, "dict_sub");
        }
    }
    Py_DECREF(left);
    Py_DECREF(right);
    pushDictLua(L, result);   /* consumes */
    return 1;
}

int pushDictLua(lua_State* L, PyObject* obj) {
    if (!PyDict_Check(obj)) {
        Py_XDECREF(obj);
        return luaL_error(L, "pushDictLua: not a dict");
    }
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_DICT_NAME)) {
        lua_pushcfunction(L, dict_len);
        lua_setfield(L, -2, "__len");
        lua_pushcfunction(L, dict_add);
        lua_setfield(L, -2, "__add");
        lua_pushcfunction(L, dict_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, dict_sub);
        lua_setfield(L, -2, "__sub");
        lua_pushcfunction(L, dict_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, dict_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, python_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertDictPython(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TTABLE) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            return NULL;
        }
        lua_pushvalue(L, index);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            PyObject* key = convertPython(L, -2);
            PyObject* value = convertPython(L, -1);
            lua_pop(L, 1);                   /* pop the value, keep the key */
            if (key == NULL || value == NULL) {
                Py_XDECREF(key);
                Py_XDECREF(value);
                lua_pop(L, 2);               /* key + table copy */
                Py_DECREF(dict);
                luapython_throw(L, "convertDictPython");
                return NULL;
            }
            /* Keys are converted like any other value, so numeric keys are
             * preserved instead of being silently dropped. Unhashable keys
             * (tables, lists) fail here with a real error. */
            int rc = PyDict_SetItem(dict, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            if (rc != 0) {
                lua_pop(L, 2);
                Py_DECREF(dict);
                luapython_throw(L, "convertDictPython");
                return NULL;
            }
        }
        lua_pop(L, 1);                       /* table copy */
        return dict;
    }
    if (isPythonDict(L, index)) {
        PyObject* dict = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(dict);
        return dict;
    }
    luaL_error(L, "convertDictPython: expected a table or dict at index %d, got %s",
               index, luaL_typename(L, index));
    return NULL;
}
