#include "luapython.h"

/* Note: strings that can be encoded to UTF-8 are turned into plain Lua
 * strings by pushStringLua(), so these metamethods only ever see Python
 * strings that failed to encode (lone surrogates, ...). They are kept
 * correct anyway because relying on "unreachable" is how bugs are born. */

#define isPythonString(L, index) (isPythonObject(L, index) && PyUnicode_Check(*(PyObject**)lua_touserdata(L, index)))

/* Fetch operand `index` (a Lua string or a wrapped Python string) as a new
 * Python reference. Returns NULL on failure. */
static PyObject* string_operand(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TSTRING) {
        size_t len;
        const char* text = lua_tolstring(L, index, &len);
        return PyUnicode_FromStringAndSize(text, (Py_ssize_t)len);
    }
    if (isPythonString(L, index)) {
        PyObject* obj = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(obj);
        return obj;
    }
    return NULL;
}

int string_concat(lua_State* L) {
    PyObject* left = string_operand(L, -2);
    PyObject* right = string_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "string_concat: attempt to concatenate %s and %s",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }
    PyObject* result = PyUnicode_Concat(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    if (result == NULL) {
        return luapython_throw(L, "string_concat");
    }
    pushStringLua(L, result);   /* consumes */
    return 1;
}

/* Lua 5.1 hands `__len` two arguments (the object and a nil), 5.2+ hands over
 * only one. Address the object as argument #1 so both work. */
int string_len(lua_State* L) {
    if (lua_type(L, 1) == LUA_TSTRING) {
        lua_pushinteger(L, (lua_Integer)lua_rawlen(L, 1));
        return 1;
    }
    if (!isPythonString(L, 1)) {
        return luaL_error(L, "string_len: attempt to get length of %s", luaL_typename(L, 1));
    }
    PyObject* str = *(PyObject**)lua_touserdata(L, 1);
    Py_ssize_t len = PyUnicode_GetLength(str);
    if (len < 0) {
        return luapython_throw(L, "string_len");
    }
    lua_pushinteger(L, (lua_Integer)len);
    return 1;
}

int string_eq(lua_State* L) {
    PyObject* left = string_operand(L, -2);
    PyObject* right = string_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "string_eq: attempt to compare %s and %s as strings",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }
    int result = PyObject_RichCompareBool(left, right, Py_EQ);
    Py_DECREF(left);
    Py_DECREF(right);
    if (result < 0) {
        return luapython_throw(L, "string_eq");
    }
    lua_pushboolean(L, result);
    return 1;
}

int string_lt(lua_State* L) {
    PyObject* left = string_operand(L, -2);
    PyObject* right = string_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "string_lt: attempt to compare %s and %s as strings",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }
    int result = PyObject_RichCompareBool(left, right, Py_LT);
    Py_DECREF(left);
    Py_DECREF(right);
    if (result < 0) {
        return luapython_throw(L, "string_lt");
    }
    lua_pushboolean(L, result);
    return 1;
}

int string_le(lua_State* L) {
    PyObject* left = string_operand(L, -2);
    PyObject* right = string_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "string_le: attempt to compare %s and %s as strings",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }
    int result = PyObject_RichCompareBool(left, right, Py_LE);
    Py_DECREF(left);
    Py_DECREF(right);
    if (result < 0) {
        return luapython_throw(L, "string_le");
    }
    lua_pushboolean(L, result);
    return 1;
}

int string_tostring(lua_State* L) {
    if (lua_type(L, -1) == LUA_TSTRING) {
        lua_pushvalue(L, -1);
        return 1;
    }
    return python_tostring(L);
}

/* pystr * n  (repetition) */
int string_mul(lua_State* L) {
    if (!isPythonString(L, -2)) {
        return luaL_error(L, "string_mul: attempt to multiply %s by %s",
                          luaL_typename(L, -2), luaL_typename(L, -1));
    }
    lua_Integer count = luaL_checkinteger(L, -1);
    if (count < 0) {
        return luaL_error(L, "string_mul: repetition count must be non-negative");
    }
    PyObject* str = *(PyObject**)lua_touserdata(L, -2);
    PyObject* py_count = PyLong_FromLongLong((long long)count);
    if (py_count == NULL) {
        return luapython_throw(L, "string_mul");
    }
    PyObject* result = PyNumber_Multiply(str, py_count);
    Py_DECREF(py_count);
    if (result == NULL) {
        return luapython_throw(L, "string_mul");
    }
    pushStringLua(L, result);   /* consumes */
    return 1;
}

int pushStringLua(lua_State* L, PyObject* obj) {
    if (!PyUnicode_Check(obj)) {
        Py_XDECREF(obj);
        return luaL_error(L, "pushStringLua: expected a Python string object");
    }
    PyObject* bytes = PyUnicode_AsEncodedString(obj, "utf-8", "surrogateescape");
    if (bytes != NULL) {
        const char* text = PyBytes_AsString(bytes);
        Py_ssize_t size = PyBytes_Size(bytes);
        lua_pushlstring(L, text, (size_t)size);
        Py_DECREF(bytes);
        Py_DECREF(obj);        /* push* consumes the reference */
        return 1;
    }
    /* Not representable as a Lua string (lone surrogates): wrap it instead of
     * silently losing the value, and clear the encoding error. */
    PyErr_Clear();
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_STRING_NAME)) {
        lua_pushcfunction(L, string_concat);
        lua_setfield(L, -2, "__concat");
        lua_pushcfunction(L, string_len);
        lua_setfield(L, -2, "__len");
        lua_pushcfunction(L, string_eq);
        lua_setfield(L, -2, "__eq");
        lua_pushcfunction(L, string_lt);
        lua_setfield(L, -2, "__lt");
        lua_pushcfunction(L, string_le);
        lua_setfield(L, -2, "__le");
        lua_pushcfunction(L, string_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, string_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertStringPython(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TSTRING) {
        size_t len;
        const char* text = lua_tolstring(L, index, &len);
        return PyUnicode_FromStringAndSize(text, (Py_ssize_t)len);
    }
    if (isPythonString(L, index)) {
        PyObject* str = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(str);
        return str;
    }
    luaL_error(L, "convertStringPython: expected a string at index %d, got %s",
               index, luaL_typename(L, index));
    return NULL;
}
