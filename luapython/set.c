#include "luapython.h"

#define isPythonSet(L, index) (isPythonObject(L, index) && PySet_Check(*(PyObject**)lua_touserdata(L, index)))

/* Fetch operand `index` as a Python set (converting a Lua table if needed).
 * Returns a new reference or NULL. */
static PyObject* set_operand(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TTABLE) {
        return convertPython(L, index);
    }
    if (isPythonSet(L, index)) {
        PyObject* set = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(set);
        return set;
    }
    return NULL;
}

/* left <op> right, where op is a PyNumber_* binary function. */
static int set_binop(lua_State* L, const char* opname,
                     PyObject* (*op)(PyObject*, PyObject*)) {
    PyObject* left = set_operand(L, -2);
    PyObject* right = set_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "set_%s: attempt to combine %s and %s",
                          opname, luaL_typename(L, -2), luaL_typename(L, -1));
    }
    PyObject* result = op(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    if (result == NULL) {
        char context[32];
        snprintf(context, sizeof context, "set %s", opname);
        return luapython_throw(L, context);
    }
    pushSetLua(L, result);   /* consumes */
    return 1;
}

/* s1 + s2 : union */
int set_add(lua_State* L) {
    return set_binop(L, "union", PyNumber_Or);
}

/* s1 * s2 : intersection (same convention as dict) */
int set_mul(lua_State* L) {
    return set_binop(L, "intersection", PyNumber_And);
}

/* s1 - s2 : difference */
int set_sub(lua_State* L) {
    return set_binop(L, "difference", PyNumber_Subtract);
}

/* s1 & s2 : intersection */
int set_band(lua_State* L) {
    return set_binop(L, "intersection", PyNumber_And);
}

/* s1 | s2 : union */
int set_bor(lua_State* L) {
    return set_binop(L, "union", PyNumber_Or);
}

/* s1 ~ s2 : symmetric difference */
int set_bxor(lua_State* L) {
    return set_binop(L, "symmetric difference", PyNumber_Xor);
}

/* Lua 5.1 hands `__len` two arguments (the object and a nil), 5.2+ hands over
 * only one. Address the object as argument #1 so both work. */
int set_len(lua_State* L) {
    if (lua_type(L, 1) == LUA_TTABLE) {
#if LUA_VERSION_NUM >= 502
        lua_len(L, 1);
#else
        lua_pushnumber(L, lua_objlen(L, 1));
#endif
        return 1;
    }
    if (!isPythonSet(L, 1)) {
        return luaL_error(L, "set_len: attempt to get length of %s", luaL_typename(L, 1));
    }
    PyObject* set = *(PyObject**)lua_touserdata(L, 1);
    lua_pushinteger(L, (lua_Integer)PySet_Size(set));
    return 1;
}

int set_index(lua_State* L) {
    return luaL_error(L, "set_index: Python sets are not subscriptable; iterate them with for");
}

int set_newindex(lua_State* L) {
    return luaL_error(L, "set_newindex: Python sets do not support item assignment");
}

int pushSetLua(lua_State* L, PyObject* obj) {
    if (!PySet_Check(obj)) {
        Py_XDECREF(obj);
        return luaL_error(L, "pushSetLua: not a set");
    }
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_SET_NAME)) {
        lua_pushcfunction(L, set_add);
        lua_setfield(L, -2, "__add");
        lua_pushcfunction(L, set_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, set_sub);
        lua_setfield(L, -2, "__sub");
        lua_pushcfunction(L, set_band);
        lua_setfield(L, -2, "__band");
        lua_pushcfunction(L, set_bor);
        lua_setfield(L, -2, "__bor");
        lua_pushcfunction(L, set_bxor);
        lua_setfield(L, -2, "__bxor");
        lua_pushcfunction(L, set_len);
        lua_setfield(L, -2, "__len");
        lua_pushcfunction(L, set_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, set_newindex);
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

PyObject* convertSetPython(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TTABLE) {
        PyObject* list = convertListPython(L, index);
        if (list == NULL) {
            return NULL;
        }
        PyObject* set = PySet_New(list);
        Py_DECREF(list);
        return set;
    }
    if (isPythonSet(L, index)) {
        PyObject* set = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(set);
        return set;
    }
    luaL_error(L, "convertSetPython: expected a table or set at index %d, got %s",
               index, luaL_typename(L, index));
    return NULL;
}
