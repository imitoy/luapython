#include "luapython.hpp"
#include <string>

static bool is_pystring(lua_State* L, int index) {
    if (!lua_isuserdata(L, index))
        return false;

    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.string");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        return is_same;
    }
    return false;
}

static const char* get_string_value(lua_State* L, int index, size_t* len = nullptr) {
    if (lua_isstring(L, index)) {
        return lua_tolstring(L, index, len);
    } else if (is_pystring(L, index)) {
        PyObject* py_str = *(PyObject**)lua_touserdata(L, index);
        if (PyUnicode_Check(py_str)) {
            const char* result = PyUnicode_AsUTF8(py_str);
            if (len)
                *len = PyUnicode_GET_LENGTH(py_str);
            return result;
        }
    }
    return nullptr;
}

int string_concat(lua_State* L) {
    if (!(lua_isstring(L, -1) || is_pystring(L, -1)) || !(lua_isstring(L, -2) || is_pystring(L, -2))) {
        luaL_error(L, "string_concat: Attempt to concatenate %s and %s", luaL_typename(L, -2), luaL_typename(L, -1));
        return 0;
    }
string_concat:

    if (lua_isstring(L, -1) && lua_isstring(L, -2)) {
        lua_pushvalue(L, -2);
        lua_pushvalue(L, -2);
        lua_concat(L, 2);
        return 1;
    }

    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;

    if (lua_isstring(L, -1)) {
        py_str1 = PyUnicode_FromString(lua_tostring(L, -1));
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_str1);
    }

    if (lua_isstring(L, -2)) {
        py_str2 = PyUnicode_FromString(lua_tostring(L, -2));
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1)
            Py_DECREF(py_str1);
        if (py_str2)
            Py_DECREF(py_str2);
        luaL_error(L, "string_concat: Failed to create Python strings");
        return 0;
    }

    PyObject* result = PyUnicode_Concat(py_str1, py_str2);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);

    if (!result) {
        luaL_error(L, "string_concat: Python string concatenation failed");
        return 0;
    }

    pushStringLua(L, result);
    return 1;
}

int string_len(lua_State* L) {
    if (!(lua_isstring(L, -1) || is_pystring(L, -1))) {
        luaL_error(L, "string_len: Attempt to get length of %s", luaL_typename(L, -1));
        return 0;
    }

    if (lua_isstring(L, -1)) {
        size_t len;
        lua_tolstring(L, -1, &len);
        lua_pushinteger(L, len);
        return 1;
    }

    PyObject* py_str = *(PyObject**)lua_touserdata(L, -1);
    Py_ssize_t len = PyUnicode_GET_LENGTH(py_str);
    lua_pushinteger(L, len);
    return 1;
}

int string_eq(lua_State* L) {
    if (!(lua_isstring(L, -1) || is_pystring(L, -1)) || !(lua_isstring(L, -2) || is_pystring(L, -2))) {
        luaL_error(L, "string_len: Attempt to compare %s and %s as strings", luaL_typename(L, -2),
                   luaL_typename(L, -1));
        return 0;
    }

    if (lua_isstring(L, -1) && lua_isstring(L, -2)) {
        const char* str1 = lua_tostring(L, 1);
        const char* str2 = lua_tostring(L, 2);
        lua_pushboolean(L, strcmp(str1, str2) == 0);
        return 1;
    }

    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;

    if (lua_isstring(L, -1)) {
        py_str1 = PyUnicode_FromString(lua_tostring(L, -1));
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_str1);
    }

    if (lua_isstring(L, -2)) {
        py_str2 = PyUnicode_FromString(lua_tostring(L, -2));
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1)
            Py_DECREF(py_str1);
        if (py_str2)
            Py_DECREF(py_str2);
        luaL_error(L, "string_len: Failed to create Python strings");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_str1, py_str2, Py_EQ);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);

    lua_pushboolean(L, result);
    return 1;
}

int string_lt(lua_State* L) {
    if (!(lua_isstring(L, -1) || is_pystring(L, -1)) || !(lua_isstring(L, -2) || is_pystring(L, -2))) {
        luaL_error(L, "string_lt: Attempt to compare %s and %s as strings", luaL_typename(L, -2), luaL_typename(L, -1));
        return 0;
    }

    if (lua_isstring(L, -1) && lua_isstring(L, -2)) {
        const char* str1 = lua_tostring(L, -2);
        const char* str2 = lua_tostring(L, -1);
        lua_pushboolean(L, strcmp(str1, str2) < 0);
        return 1;
    }

    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;

    if (lua_isstring(L, -1)) {
        py_str1 = PyUnicode_FromString(lua_tostring(L, -1));
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_str1);
    }

    if (lua_isstring(L, -2)) {
        py_str2 = PyUnicode_FromString(lua_tostring(L, -2));
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1)
            Py_DECREF(py_str1);
        if (py_str2)
            Py_DECREF(py_str2);
        luaL_error(L, "string_lt: Failed to create Python strings");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_str2, py_str1, Py_LT);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);

    lua_pushboolean(L, result);
    return 1;
}

int string_le(lua_State* L) {
    if (!(lua_isstring(L, -1) || is_pystring(L, -1)) || !(lua_isstring(L, -2) || is_pystring(L, -2))) {
        luaL_error(L, "string_le: Attempt to compare %s and %s as strings", luaL_typename(L, -1), luaL_typename(L, -2));
        return 0;
    }

    if (lua_isstring(L, -1) && lua_isstring(L, -2)) {
        const char* str1 = lua_tostring(L, -2);
        const char* str2 = lua_tostring(L, -1);
        lua_pushboolean(L, strcmp(str1, str2) <= 0);
        return 1;
    }

    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;

    if (lua_isstring(L, -1)) {
        py_str1 = PyUnicode_FromString(lua_tostring(L, -1));
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_str1);
    }

    if (lua_isstring(L, -2)) {
        py_str2 = PyUnicode_FromString(lua_tostring(L, -2));
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1)
            Py_DECREF(py_str1);
        if (py_str2)
            Py_DECREF(py_str2);
        luaL_error(L, "string_le: Failed to create Python strings");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_str2, py_str1, Py_LE);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);

    lua_pushboolean(L, result);
    return 1;
}

int string_tostring(lua_State* L) {
    if (!(lua_isstring(L, -1) || is_pystring(L, -1))) {
        luaL_error(L, "string_tostring: Attempt to convert a %s value to string", luaL_typename(L, -1));
        return 0;
    }

    if (lua_isstring(L, -1)) {
        lua_pushvalue(L, -1);
        return 1;
    }

    PyObject* py_str = *(PyObject**)lua_touserdata(L, -1);
    const char* str = PyUnicode_AsUTF8(py_str);
    if (!str) {
        luaL_error(L, "string_tostring: Failed to convert Python string to Lua string");
        return 0;
    }

    lua_pushstring(L, str);
    return 1;
}

int string_mul(lua_State* L) {
    if (!(lua_isstring(L, -2) || is_pystring(L, -2))) {
        luaL_error(L, "string_mul: Attempt to multiply a %s value", luaL_typename(L, -2));
        return 0;
    }

    if (!lua_isnumber(L, -1)) {
        luaL_error(L, "string_mul: Attempt to multiply string by %s", luaL_typename(L, -1));
        return 0;
    }

    int count = lua_tointeger(L, -1);
    if (count < 0) {
        luaL_error(L, "string_mul: String repetition count must be non-negative");
        return 0;
    }

    if (lua_isstring(L, -2)) {
        const char* str = lua_tostring(L, -2);
        std::string result;
        for (int i = 0; i < count; i++) {
            result += str;
        }
        lua_pushstring(L, result.c_str());
        return 1;
    }

    PyObject* py_str = *(PyObject**)lua_touserdata(L, -2);
    PyObject* py_count = PyLong_FromLong(count);

    if (!py_count) {
        luaL_error(L, "string_mul: Failed to create Python integer");
        return 0;
    }

    PyObject* result = PyNumber_Multiply(py_str, py_count);
    Py_DECREF(py_count);

    if (!result) {
        luaL_error(L, "string_mul: Python string repetition failed");
        return 0;
    }

    pushStringLua(L, result);
    return 1;
}

int pushStringLua(lua_State* L, PyObject* string) {
    if (!PyUnicode_Check(string)) {
        luaL_error(L, "pushStringLua: Expected a Python string object");
        return 1;
    }
    const char* str = PyUnicode_AsUTF8(string);
    if (!PyErr_Occurred()) {
        lua_pushstring(L, str);
        return 1;
    }
    void* userdata = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)userdata = string;
    Py_INCREF(string);
    lua_createtable(L, 0, 8);
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
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertStringPython(lua_State* L, int index) {
    if (lua_isstring(L, index)) {
        return PyUnicode_FromString(lua_tostring(L, index));
    } else if (is_pystring(L, index)) {
        PyObject* py_str = *(PyObject**)lua_touserdata(L, index);
        Py_INCREF(py_str);
        return py_str;
    }
    luaL_error(L, "convertStringPython: Expected a string or Python string object at index %d", index);
    return nullptr;
}