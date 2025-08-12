#include "luapython.hpp"

// 检查是否为Python列表对象
static bool is_pylist(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) return false;
    
    // 获取元表
    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.list");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2); // 弹出两个元表
        return is_same;
    }
    return false;
}

// 将Lua表转换为Python列表（假设Lua表使用整数索引，从1开始）
static PyObject* lua_table_to_pylist(lua_State* L, int index) {
    if (!lua_istable(L, index)) return nullptr;
    
    // 计算列表长度（取最大整数索引）
    lua_len(L, index);
    lua_Integer len = lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    PyObject* py_list = PyList_New(len);
    if (!py_list) return nullptr;
    
    for (lua_Integer i = 1; i <= len; ++i) {
        lua_pushinteger(L, i);
        lua_gettable(L, index);
        
        PyObject* py_value = convertPython(L, -1);
        if (py_value) {
            PyList_SetItem(py_list, i - 1, py_value); // 转移所有权，无需DECREF
        } else {
            Py_DECREF(py_list);
            lua_pop(L, 1);
            return nullptr;
        }
        lua_pop(L, 1);
    }
    
    return py_list;
}

// 将Python列表转换为Lua表
static int pylist_to_lua_table(lua_State* L, PyObject* py_list) {
    if (!PyList_Check(py_list)) return 0;
    
    Py_ssize_t len = PyList_Size(py_list);
    lua_newtable(L);
    
    for (Py_ssize_t i = 0; i < len; ++i) {
        PyObject* py_value = PyList_GetItem(py_list, i); // 不增加引用计数
        lua_pushlightuserdata(L, py_value);
        pushLua(L, py_value);
        lua_seti(L, -2, i + 1); // Lua索引从1开始
    }
    
    return 1;
}

// 列表长度
int list_len(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pylist(L, 1))) {
        luaL_error(L, "Attempt to get length of %s", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        lua_len(L, 1);
        return 1;
    }

    // Python列表长度
    PyObject* py_list = *(PyObject**)lua_touserdata(L, 1);
    Py_ssize_t len = PyList_Size(py_list);
    lua_pushinteger(L, len);
    return 1;
}

// 列表相等比较
int list_eq(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pylist(L, 1)) || 
        !(lua_istable(L, 2) || is_pylist(L, 2))) {
        luaL_error(L, "Attempt to compare %s and %s as lists", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python列表进行比较
    PyObject* py_list1 = nullptr;
    PyObject* py_list2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_list1 = lua_table_to_pylist(L, 1);
    } else {
        py_list1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_list1);
    }
    
    if (lua_istable(L, 2)) {
        py_list2 = lua_table_to_pylist(L, 2);
    } else {
        py_list2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_list2);
    }

    if (!py_list1 || !py_list2) {
        if (py_list1) Py_DECREF(py_list1);
        if (py_list2) Py_DECREF(py_list2);
        luaL_error(L, "Failed to create Python lists");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_list1, py_list2, Py_EQ);
    Py_DECREF(py_list1);
    Py_DECREF(py_list2);
    
    lua_pushboolean(L, result);
    return 1;
}

int list_index(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pylist(L, 1))) {
        luaL_error(L, "Attempt to index %s", luaL_typename(L, 1));
        return 0;
    }

    if (!lua_isinteger(L, 2)) {
        luaL_error(L, "List index must be an integer");
        return 0;
    }

    lua_Integer idx = lua_tointeger(L, 2);
    
    if (lua_istable(L, 1)) {
        // 对于Lua表，直接使用索引
        lua_pushinteger(L, idx);
        lua_gettable(L, 1);
        return 1;
    }

    // Python列表索引访问（转换为0基索引）
    PyObject* py_list = *(PyObject**)lua_touserdata(L, 1);
    Py_ssize_t len = PyList_Size(py_list);
    Py_ssize_t py_idx = idx - 1; // 转换为0基索引
    
    if (py_idx < 0 || py_idx >= len) {
        lua_pushnil(L); // 索引越界返回nil
        return 1;
    }

    PyObject* py_value = PyList_GetItem(py_list, py_idx); // 不增加引用计数
    lua_pushlightuserdata(L, py_value);
    pushLua(L, py_value);
    return 1;
}

// 列表索引赋值
int list_newindex(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pylist(L, 1))) {
        luaL_error(L, "Attempt to assign to %s", luaL_typename(L, 1));
        return 0;
    }

    if (!lua_isinteger(L, 2)) {
        luaL_error(L, "List index must be an integer");
        return 0;
    }

    lua_Integer idx = lua_tointeger(L, 2);
    
    if (lua_istable(L, 1)) {
        // 对于Lua表，标准赋值
        lua_pushinteger(L, idx);
        lua_pushvalue(L, 3);
        lua_settable(L, 1);
        return 0;
    }

    // Python列表索引赋值
    PyObject* py_list = *(PyObject**)lua_touserdata(L, 1);
    Py_ssize_t len = PyList_Size(py_list);
    Py_ssize_t py_idx = idx - 1; // 转换为0基索引
    
    if (py_idx < 0 || py_idx >= len) {
        luaL_error(L, "List index out of range");
        return 0;
    }

    PyObject* py_value = convertPython(L, 3);
    if (!py_value) {
        luaL_error(L, "Invalid value type for list assignment");
        return 0;
    }

    // 替换列表元素（先释放旧元素）
    PyObject* old_value = PyList_GetItem(py_list, py_idx);
    Py_INCREF(old_value); // 手动增加引用计数以便释放
    int result = PyList_SetItem(py_list, py_idx, py_value);
    Py_DECREF(old_value); // 释放旧元素
    
    if (result < 0) {
        Py_DECREF(py_value);
        luaL_error(L, "Failed to set list item");
        return 0;
    }
    
    return 0;
}

// 列表转换为字符串
int list_tostring(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pylist(L, 1))) {
        luaL_error(L, "Attempt to convert a %s value to string", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        lua_pushstring(L, "list");
        return 1;
    }

    // Python列表转换为字符串
    PyObject* py_list = *(PyObject**)lua_touserdata(L, 1);
    PyObject* str_repr = PyObject_Str(py_list);
    if (!str_repr) {
        luaL_error(L, "Failed to convert Python list to string");
        return 0;
    }
    
    const char* str = PyUnicode_AsUTF8(str_repr);
    if (!str) {
        Py_DECREF(str_repr);
        luaL_error(L, "Failed to get string representation");
        return 0;
    }
    
    lua_pushstring(L, str);
    Py_DECREF(str_repr);
    return 1;
}

// 列表拼接（+操作）
int list_add(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pylist(L, 1)) || 
        !(lua_istable(L, 2) || is_pylist(L, 2))) {
        luaL_error(L, "Attempt to concatenate %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python列表
    PyObject* py_list1 = nullptr;
    PyObject* py_list2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_list1 = lua_table_to_pylist(L, 1);
    } else {
        py_list1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_list1);
    }
    
    if (lua_istable(L, 2)) {
        py_list2 = lua_table_to_pylist(L, 2);
    } else {
        py_list2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_list2);
    }

    if (!py_list1 || !py_list2) {
        if (py_list1) Py_DECREF(py_list1);
        if (py_list2) Py_DECREF(py_list2);
        luaL_error(L, "Failed to create Python lists");
        return 0;
    }

    // 拼接列表
    PyObject* result = PyList_New(PyList_Size(py_list1) + PyList_Size(py_list2));
    if (!result) {
        Py_DECREF(py_list1);
        Py_DECREF(py_list2);
        luaL_error(L, "Failed to create new list");
        return 0;
    }
    
    // 复制第一个列表元素
    for (Py_ssize_t i = 0; i < PyList_Size(py_list1); ++i) {
        PyObject* item = PyList_GetItem(py_list1, i);
        Py_INCREF(item);
        PyList_SetItem(result, i, item);
    }
    
    // 复制第二个列表元素
    Py_ssize_t offset = PyList_Size(py_list1);
    for (Py_ssize_t i = 0; i < PyList_Size(py_list2); ++i) {
        PyObject* item = PyList_GetItem(py_list2, i);
        Py_INCREF(item);
        PyList_SetItem(result, offset + i, item);
    }

    Py_DECREF(py_list1);
    Py_DECREF(py_list2);

    lua_pushlightuserdata(L, result);
    pushListLua(L, result);
    return 1;
}

// 列表重复（*操作）
int list_mul(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pylist(L, 1)) || !lua_isinteger(L, 2)) {
        luaL_error(L, "Attempt to repeat %s with non-integer", luaL_typename(L, 1));
        return 0;
    }

    lua_Integer n = lua_tointeger(L, 2);
    if (n < 0) {
        luaL_error(L, "Repeat count must be non-negative");
        return 0;
    }

    // 转换为Python列表
    PyObject* py_list = nullptr;
    if (lua_istable(L, 1)) {
        py_list = lua_table_to_pylist(L, 1);
    } else {
        py_list = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_list);
    }

    if (!py_list) {
        luaL_error(L, "Failed to create Python list");
        return 0;
    }

    // 重复列表
    PyObject* result = PySequence_Repeat(py_list, n);
    Py_DECREF(py_list);
    
    if (!result) {
        luaL_error(L, "Failed to repeat list");
        return 0;
    }

    lua_pushlightuserdata(L, result);
    pushListLua(L, result);
    return 1;
}

// 设置列表元表
int pushListLua(lua_State* L, PyObject* list) { 
    if (lua_touserdata(L, -1) != list) {
        luaL_error(L, "Failed to set metatable for list");
        return 0;
    }
    void* ud = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)ud = list;
    Py_INCREF(list);
    lua_createtable(L, 0, 8);
    lua_pushcfunction(L, list_len);
    lua_setfield(L, -2, "__len");
    lua_pushcfunction(L, list_eq);
    lua_setfield(L, -2, "__eq");
    lua_pushcfunction(L, list_add);
    lua_setfield(L, -2, "__add");
    lua_pushcfunction(L, list_mul);
    lua_setfield(L, -2, "__mul");
    lua_pushcfunction(L, list_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, list_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, list_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}

// 转换为Python列表
PyObject* convertListPython(lua_State* L, int index) {
    if (lua_istable(L, index)) {
        return lua_table_to_pylist(L, index);
    } else if (is_pylist(L, index)) {
        PyObject* py_list = *(PyObject**)lua_touserdata(L, index);
        Py_INCREF(py_list);
        return py_list;
    }
    luaL_error(L, "Attempt to convert a %s value to Python list", luaL_typename(L, index));
    return nullptr; // 不会到达这里
}