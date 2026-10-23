#include "luapython.h"

#define isPythonList(L, index) (isPythonObject(L, index) && PyList_Check(*(PyObject**)lua_touserdata(L, index)))

/* Lua uses 1-based indices, Python 0-based. Mapping:
 *   l[1]  -> py[0]
 *   l[n]  -> py[n-1]
 *   l[-1] -> py[-len] ... py[len-1]   (Python style negative index)
 *   l[0]  -> out of range (never a valid Lua sequence index)
 * Returns the Python index, or -1 when `idx` is out of range for `len`. */
static Py_ssize_t resolve_index(lua_Integer idx, Py_ssize_t len) {
    Py_ssize_t py_idx;
    if (idx > 0) {
        py_idx = (Py_ssize_t)idx - 1;
    } else if (idx < 0) {
        py_idx = len + (Py_ssize_t)idx;
    } else {
        return -1;
    }
    if (py_idx < 0 || py_idx >= len) {
        return -1;
    }
    return py_idx;
}

/* Lua 5.1 hands `__len` two arguments (the object and a nil), 5.2+ hands over
 * only one. Address the object as argument #1 so both work. */
int list_len(lua_State* L) {
    if (lua_type(L, 1) == LUA_TTABLE) {
#if LUA_VERSION_NUM >= 502
        lua_len(L, 1);
#else
        lua_pushnumber(L, lua_objlen(L, 1));
#endif
        return 1;
    }
    if (!isPythonList(L, 1)) {
        return luaL_error(L, "list_len: attempt to get length of %s", luaL_typename(L, 1));
    }
    PyObject* list = *(PyObject**)lua_touserdata(L, 1);
    lua_pushinteger(L, (lua_Integer)PyList_Size(list));
    return 1;
}

int list_index(lua_State* L) {
    if (lua_type(L, -2) == LUA_TTABLE) {
        lua_pushvalue(L, -1);
        lua_gettable(L, -2);
        return 1;
    }
    if (!isPythonList(L, -2)) {
        return luaL_error(L, "list_index: attempt to index %s", luaL_typename(L, -2));
    }
    lua_Integer idx = luaL_checkinteger(L, -1);
    PyObject* list = *(PyObject**)lua_touserdata(L, -2);
    Py_ssize_t len = PyList_Size(list);
    Py_ssize_t py_idx = resolve_index(idx, len);
    if (py_idx < 0) {
        lua_pushnil(L);
        return 1;
    }
    PyObject* value = PyList_GetItem(list, py_idx);   /* borrowed */
    Py_XINCREF(value);
    pushLua(L, value);                                /* consumes */
    return 1;
}

int list_newindex(lua_State* L) {
    if (!isPythonList(L, -3)) {
        return luaL_error(L, "list_newindex: attempt to assign to %s", luaL_typename(L, -3));
    }
    lua_Integer idx = luaL_checkinteger(L, -2);
    PyObject* list = *(PyObject**)lua_touserdata(L, -3);
    Py_ssize_t len = PyList_Size(list);
    Py_ssize_t py_idx = resolve_index(idx, len);
    if (py_idx < 0) {
        /* luaL_error goes through lua_pushvfstring, which only understands
         * %d %f %p %s %c %U %I -- no length modifiers. Format separately. */
        char msg[128];
        snprintf(msg, sizeof msg,
                 "list_newindex: index %lld out of range for a list of length %lld",
                 (long long)idx, (long long)len);
        return luaL_error(L, "%s", msg);
    }
    PyObject* value = convertPython(L, -1);
    if (value == NULL) {
        return luapython_throw(L, "list_newindex: cannot convert value");
    }
    /* PyList_SetItem() steals the reference: no Py_DECREF afterwards. */
    int rc = PyList_SetItem(list, py_idx, value);
    if (rc != 0) {
        /* PyList_SetItem() already released `value` on its own error paths,
         * so decref'ing again here would be a double free. */
        return luapython_throw(L, "list_newindex");
    }
    return 0;
}

int list_add(lua_State* L) {
    PyObject* left = NULL;
    PyObject* right = NULL;

    if (lua_type(L, -2) == LUA_TTABLE) {
        left = convertPython(L, -2);
    } else if (isPythonList(L, -2)) {
        left = *(PyObject**)lua_touserdata(L, -2);
        Py_XINCREF(left);
    }
    if (lua_type(L, -1) == LUA_TTABLE) {
        right = convertPython(L, -1);
    } else if (isPythonList(L, -1)) {
        right = *(PyObject**)lua_touserdata(L, -1);
        Py_XINCREF(right);
    }
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "list_add: attempt to add %s and %s",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }

    Py_ssize_t left_len = PyList_Size(left);
    Py_ssize_t right_len = PyList_Size(right);
    PyObject* result = PyList_New(left_len + right_len);
    if (result == NULL) {
        Py_DECREF(left);
        Py_DECREF(right);
        return luapython_throw(L, "list_add");
    }
    for (Py_ssize_t i = 0; i < left_len; i++) {
        PyObject* item = PyList_GetItem(left, i);
        Py_INCREF(item);
        PyList_SetItem(result, i, item);
    }
    for (Py_ssize_t i = 0; i < right_len; i++) {
        PyObject* item = PyList_GetItem(right, i);
        Py_INCREF(item);
        PyList_SetItem(result, left_len + i, item);
    }
    Py_DECREF(left);
    Py_DECREF(right);
    pushListLua(L, result);   /* consumes */
    return 1;
}

int list_mul(lua_State* L) {
    if (!isPythonList(L, -2)) {
        return luaL_error(L, "list_mul: attempt to repeat %s", luaL_typename(L, -2));
    }
    lua_Integer n = luaL_checkinteger(L, -1);
    if (n < 0) {
        n = 0;   /* Python: [1] * -1 == [] */
    }
    PyObject* list = *(PyObject**)lua_touserdata(L, -2);
    PyObject* result = PySequence_Repeat(list, (Py_ssize_t)n);
    if (result == NULL) {
        return luapython_throw(L, "list_mul");
    }
    pushListLua(L, result);   /* consumes */
    return 1;
}

int pushListLua(lua_State* L, PyObject* obj) {
    if (!PyList_Check(obj)) {
        Py_XDECREF(obj);
        return luaL_error(L, "pushListLua: not a list");
    }
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_LIST_NAME)) {
        lua_pushcfunction(L, list_len);
        lua_setfield(L, -2, "__len");
        lua_pushcfunction(L, list_add);
        lua_setfield(L, -2, "__add");
        lua_pushcfunction(L, list_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, list_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, list_newindex);
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

PyObject* convertListPython(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TTABLE) {
        lua_Integer len = (lua_Integer)lua_rawlen(L, index);

        /* A table like {1, 2, a = 3} has no unambiguous Python equivalent.
         * Converting just the array part would drop data silently, so make
         * the caller choose luapython.list{} or luapython.dict{}. */
        lua_Integer total = 0;
        lua_pushvalue(L, index);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            total++;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        if (total != len) {
            char msg[192];
            snprintf(msg, sizeof msg,
                     "convertListPython: table has %lld keys but only %lld sequential "
                     "items; use luapython.list{} or luapython.dict{} to say what you mean",
                     (long long)total, (long long)len);
            luaL_error(L, "%s", msg);
            return NULL;
        }

        PyObject* list = PyList_New((Py_ssize_t)len);
        if (list == NULL) {
            return NULL;
        }
        for (lua_Integer i = 1; i <= len; i++) {
            lua_rawgeti(L, index, (int)i);
            PyObject* item = convertPython(L, -1);
            lua_pop(L, 1);
            if (item == NULL) {
                Py_DECREF(list);
                return NULL;
            }
            PyList_SetItem(list, i - 1, item);   /* steals */
        }
        return list;
    }
    if (isPythonList(L, index)) {
        PyObject* list = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(list);
        return list;
    }
    luaL_error(L, "convertListPython: expected a table or list at index %d, got %s",
               index, luaL_typename(L, index));
    return NULL;
}
