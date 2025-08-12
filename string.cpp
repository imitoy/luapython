#include <string>
#include "luapython.hpp"

// 检查是否为Python字符串对象
static bool is_pystring(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) return false;
    
    // 获取元表
    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.string");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2); // 弹出两个元表
        return is_same;
    }
    return false;
}

// 获取字符串值（支持Lua字符串和Python字符串对象）
static const char* get_string_value(lua_State* L, int index, size_t* len = nullptr) {
    if (lua_isstring(L, index)) {
        return lua_tolstring(L, index, len);
    } else if (is_pystring(L, index)) {
        PyObject* py_str = *(PyObject**)lua_touserdata(L, index);
        if (PyUnicode_Check(py_str)) {
            const char* result = PyUnicode_AsUTF8(py_str);
            if (len) *len = PyUnicode_GET_LENGTH(py_str);
            return result;
        }
    }
    return nullptr;
}

// 字符串连接操作
int string_concat(lua_State* L) {
    // 检查两个操作数是否都是字符串类型
    if (!(lua_isstring(L, 1) || is_pystring(L, 1)) || 
        !(lua_isstring(L, 2) || is_pystring(L, 2))) {
        luaL_error(L, "Attempt to concatenate %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 获取两个字符串
    size_t len1, len2;
    const char* str1 = get_string_value(L, 1, &len1);
    const char* str2 = get_string_value(L, 2, &len2);
    
    if (!str1 || !str2) {
        luaL_error(L, "Failed to get string values");
        return 0;
    }

    // 如果都是Lua字符串，直接连接
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        lua_pushlstring(L, str1, len1);
        lua_pushlstring(L, str2, len2);
        lua_concat(L, 2);
        return 1;
    }

    // 转换为Python字符串进行连接
    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;
    
    if (lua_isstring(L, 1)) {
        py_str1 = PyUnicode_FromString(str1);
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_str1);
    }
    
    if (lua_isstring(L, 2)) {
        py_str2 = PyUnicode_FromString(str2);
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1) Py_DECREF(py_str1);
        if (py_str2) Py_DECREF(py_str2);
        luaL_error(L, "Failed to create Python strings");
        return 0;
    }

    PyObject* result = PyUnicode_Concat(py_str1, py_str2);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);
    
    if (!result) {
        luaL_error(L, "Python string concatenation failed");
        return 0;
    }

    pushStringLua(L, result);
    return 1;
}

// 字符串长度
int string_len(lua_State* L) {
    if (!(lua_isstring(L, 1) || is_pystring(L, 1))) {
        luaL_error(L, "Attempt to get length of %s", luaL_typename(L, 1));
        return 0;
    }

    if (lua_isstring(L, 1)) {
        size_t len;
        lua_tolstring(L, 1, &len);
        lua_pushinteger(L, len);
        return 1;
    }

    // Python字符串长度
    PyObject* py_str = *(PyObject**)lua_touserdata(L, 1);
    Py_ssize_t len = PyUnicode_GET_LENGTH(py_str);
    lua_pushinteger(L, len);
    return 1;
}

// 字符串相等比较
int string_eq(lua_State* L) {
    if (!(lua_isstring(L, 1) || is_pystring(L, 1)) || 
        !(lua_isstring(L, 2) || is_pystring(L, 2))) {
        luaL_error(L, "Attempt to compare %s and %s as strings", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 如果都是Lua字符串，直接比较
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        const char* str1 = lua_tostring(L, 1);
        const char* str2 = lua_tostring(L, 2);
        lua_pushboolean(L, strcmp(str1, str2) == 0);
        return 1;
    }

    // 转换为Python字符串进行比较
    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;
    
    if (lua_isstring(L, 1)) {
        py_str1 = PyUnicode_FromString(lua_tostring(L, 1));
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_str1);
    }
    
    if (lua_isstring(L, 2)) {
        py_str2 = PyUnicode_FromString(lua_tostring(L, 2));
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1) Py_DECREF(py_str1);
        if (py_str2) Py_DECREF(py_str2);
        luaL_error(L, "Failed to create Python strings");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_str1, py_str2, Py_EQ);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);
    
    lua_pushboolean(L, result);
    return 1;
}

// 字符串小于比较
int string_lt(lua_State* L) {
    if (!(lua_isstring(L, 1) || is_pystring(L, 1)) || 
        !(lua_isstring(L, 2) || is_pystring(L, 2))) {
        luaL_error(L, "Attempt to compare %s and %s as strings", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 如果都是Lua字符串，直接比较
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        const char* str1 = lua_tostring(L, 1);
        const char* str2 = lua_tostring(L, 2);
        lua_pushboolean(L, strcmp(str1, str2) < 0);
        return 1;
    }

    // 转换为Python字符串进行比较
    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;
    
    if (lua_isstring(L, 1)) {
        py_str1 = PyUnicode_FromString(lua_tostring(L, 1));
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_str1);
    }
    
    if (lua_isstring(L, 2)) {
        py_str2 = PyUnicode_FromString(lua_tostring(L, 2));
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1) Py_DECREF(py_str1);
        if (py_str2) Py_DECREF(py_str2);
        luaL_error(L, "Failed to create Python strings");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_str1, py_str2, Py_LT);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);
    
    lua_pushboolean(L, result);
    return 1;
}

// 字符串小于等于比较
int string_le(lua_State* L) {
    if (!(lua_isstring(L, 1) || is_pystring(L, 1)) || 
        !(lua_isstring(L, 2) || is_pystring(L, 2))) {
        luaL_error(L, "Attempt to compare %s and %s as strings", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 如果都是Lua字符串，直接比较
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        const char* str1 = lua_tostring(L, 1);
        const char* str2 = lua_tostring(L, 2);
        lua_pushboolean(L, strcmp(str1, str2) <= 0);
        return 1;
    }

    // 转换为Python字符串进行比较
    PyObject* py_str1 = nullptr;
    PyObject* py_str2 = nullptr;
    
    if (lua_isstring(L, 1)) {
        py_str1 = PyUnicode_FromString(lua_tostring(L, 1));
    } else {
        py_str1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_str1);
    }
    
    if (lua_isstring(L, 2)) {
        py_str2 = PyUnicode_FromString(lua_tostring(L, 2));
    } else {
        py_str2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_str2);
    }

    if (!py_str1 || !py_str2) {
        if (py_str1) Py_DECREF(py_str1);
        if (py_str2) Py_DECREF(py_str2);
        luaL_error(L, "Failed to create Python strings");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_str1, py_str2, Py_LE);
    Py_DECREF(py_str1);
    Py_DECREF(py_str2);
    
    lua_pushboolean(L, result);
    return 1;
}

// 字符串转换为字符串
int string_tostring(lua_State* L) {
    if (!(lua_isstring(L, 1) || is_pystring(L, 1))) {
        luaL_error(L, "Attempt to convert a %s value to string", luaL_typename(L, 1));
        return 0;
    }

    if (lua_isstring(L, 1)) {
        // 已经是Lua字符串，直接返回
        return 1;
    }

    // Python字符串转换为Lua字符串
    PyObject* py_str = *(PyObject**)lua_touserdata(L, 1);
    const char* str = PyUnicode_AsUTF8(py_str);
    if (!str) {
        luaL_error(L, "Failed to convert Python string to Lua string");
        return 0;
    }
    
    lua_pushstring(L, str);
    return 1;
}

// 字符串重复操作
int string_mul(lua_State* L) {
    if (!(lua_isstring(L, 1) || is_pystring(L, 1))) {
        luaL_error(L, "Attempt to multiply a %s value", luaL_typename(L, 1));
        return 0;
    }
    
    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Attempt to multiply string by %s", luaL_typename(L, 2));
        return 0;
    }

    int count = lua_tointeger(L, 2);
    if (count < 0) {
        luaL_error(L, "String repetition count must be non-negative");
        return 0;
    }

    if (lua_isstring(L, 1)) {
        // Lua字符串重复
        const char* str = lua_tostring(L, 1);
        std::string result;
        for (int i = 0; i < count; i++) {
            result += str;
        }
        lua_pushstring(L, result.c_str());
        return 1;
    }

    // Python字符串重复
    PyObject* py_str = *(PyObject**)lua_touserdata(L, 1);
    PyObject* py_count = PyLong_FromLong(count);
    
    if (!py_count) {
        luaL_error(L, "Failed to create Python integer");
        return 0;
    }

    PyObject* result = PyNumber_Multiply(py_str, py_count);
    Py_DECREF(py_count);
    
    if (!result) {
        luaL_error(L, "Python string repetition failed");
        return 0;
    }

    pushStringLua(L, result);
    return 1;
}

int pushStringLua(lua_State*L, PyObject* string) {
    if(!PyUnicode_Check(string)){
        luaL_error(L, "Expected a Python string object");
        return 1;
    }
    const char* str = PyUnicode_AsUTF8(string);
    if(!PyErr_Occurred()){
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
    luaL_error(L, "Expected a string or Python string object at index %d", index);
    return nullptr; // 不会到达这里
}