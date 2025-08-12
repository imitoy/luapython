#include "luapython.hpp"

// 检查是否为Python集合对象
static bool is_pyset(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) return false;
    
    // 获取元表
    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.set");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2); // 弹出两个元表
        return is_same;
    }
    return false;
}

// 将Lua表转换为Python集合
static PyObject* lua_table_to_pyset(lua_State* L, int index) {
    if (!lua_istable(L, index)) return nullptr;
    
    PyObject* py_set = PySet_New(nullptr);
    if (!py_set) return nullptr;
    
    lua_pushnil(L); // 第一个key
    while (lua_next(L, index - 1) != 0) {
        // key在-2，value在-1
        PyObject* py_item = nullptr;
        
        if (lua_isstring(L, -1)) {
            py_item = PyUnicode_FromString(lua_tostring(L, -1));
        } else if (lua_isnumber(L, -1)) {
            py_item = PyFloat_FromDouble(lua_tonumber(L, -1));
        } else if (lua_isboolean(L, -1)) {
            py_item = lua_toboolean(L, -1) ? Py_True : Py_False;
            Py_INCREF(py_item);
        } else if (lua_isuserdata(L, -1)) {
            py_item = *(PyObject**)lua_touserdata(L, -1);
            Py_INCREF(py_item);
        } else {
            lua_pop(L, 1); // 弹出value
            continue; // 跳过不支持的类型
        }
        
        if (py_item) {
            int result = PySet_Add(py_set, py_item);
            Py_DECREF(py_item);
            if (result < 0) {
                Py_DECREF(py_set);
                return nullptr;
            }
        }
        
        lua_pop(L, 1); // 弹出value，保留key用于下一次迭代
    }
    
    return py_set;
}

// 将Python集合转换为Lua表
static int pyset_to_lua_table(lua_State* L, PyObject* py_set) {
    if (!PySet_Check(py_set)) return 0;
    
    lua_newtable(L);
    PyObject* iterator = PyObject_GetIter(py_set);
    if (!iterator) return 0;
    
    PyObject* item;
    int index = 1;
    while ((item = PyIter_Next(iterator)) != nullptr) {
        if (PyUnicode_Check(item)) {
            const char* str = PyUnicode_AsUTF8(item);
            if (str) lua_pushstring(L, str);
            else lua_pushnil(L);
        } else if (PyLong_Check(item)) {
            long value = PyLong_AsLong(item);
            lua_pushinteger(L, value);
        } else if (PyFloat_Check(item)) {
            double value = PyFloat_AsDouble(item);
            lua_pushnumber(L, value);
        } else if (item == Py_True) {
            lua_pushboolean(L, true);
        } else if (item == Py_False) {
            lua_pushboolean(L, false);
        } else {
            // 对于其他类型，转换为字符串表示
            PyObject* str_repr = PyObject_Str(item);
            if (str_repr) {
                const char* str = PyUnicode_AsUTF8(str_repr);
                if (str) lua_pushstring(L, str);
                else lua_pushnil(L);
                Py_DECREF(str_repr);
            } else {
                lua_pushnil(L);
            }
        }
        
        lua_rawseti(L, -2, index++);
        Py_DECREF(item);
    }
    
    Py_DECREF(iterator);
    return 1;
}

// 集合长度
int set_len(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1))) {
        luaL_error(L, "Attempt to get length of %s", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        lua_len(L, 1);
        return 1;
    }

    // Python集合长度
    PyObject* py_set = *(PyObject**)lua_touserdata(L, 1);
    Py_ssize_t len = PySet_Size(py_set);
    lua_pushinteger(L, len);
    return 1;
}

// 集合相等比较
int set_eq(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1)) || 
        !(lua_istable(L, 2) || is_pyset(L, 2))) {
        luaL_error(L, "Attempt to compare %s and %s as sets", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python集合进行比较
    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_set1 = lua_table_to_pyset(L, 1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_set1);
    }
    
    if (lua_istable(L, 2)) {
        py_set2 = lua_table_to_pyset(L, 2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1) Py_DECREF(py_set1);
        if (py_set2) Py_DECREF(py_set2);
        luaL_error(L, "Failed to create Python sets");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_set1, py_set2, Py_EQ);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);
    
    lua_pushboolean(L, result);
    return 1;
}

// 集合索引访问
int set_index(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1))) {
        luaL_error(L, "Attempt to index %s", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        // 对于Lua表，使用标准索引操作
        lua_pushvalue(L, 2); // 复制key
        lua_gettable(L, 1);
        return 1;
    }

    // Python集合不支持索引访问
    luaL_error(L, "Set objects do not support indexing");
    return 0;
}

// 集合索引赋值
int set_newindex(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1))) {
        luaL_error(L, "Attempt to assign to %s", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        // 对于Lua表，使用标准赋值操作
        lua_pushvalue(L, 2); // key
        lua_pushvalue(L, 3); // value
        lua_settable(L, 1);
        return 0;
    }

    // Python集合不支持索引赋值
    luaL_error(L, "Set objects do not support assignment");
    return 0;
}

// 集合转换为字符串
int set_tostring(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1))) {
        luaL_error(L, "Attempt to convert a %s value to string", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        // 对于Lua表，返回表的字符串表示
        lua_pushstring(L, "table");
        return 1;
    }

    // Python集合转换为字符串
    PyObject* py_set = *(PyObject**)lua_touserdata(L, 1);
    PyObject* str_repr = PyObject_Str(py_set);
    if (!str_repr) {
        luaL_error(L, "Failed to convert Python set to string");
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

// 集合并集操作
int set_add(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1)) || 
        !(lua_istable(L, 2) || is_pyset(L, 2))) {
        luaL_error(L, "Attempt to perform union on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python集合进行并集操作
    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_set1 = lua_table_to_pyset(L, 1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_set1);
    }
    
    if (lua_istable(L, 2)) {
        py_set2 = lua_table_to_pyset(L, 2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1) Py_DECREF(py_set1);
        if (py_set2) Py_DECREF(py_set2);
        luaL_error(L, "Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_Or(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);
    
    if (!result) {
        luaL_error(L, "Python set union failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

// 集合交集操作
int set_mul(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1)) || 
        !(lua_istable(L, 2) || is_pyset(L, 2))) {
        luaL_error(L, "Attempt to perform intersection on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python集合进行交集操作
    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_set1 = lua_table_to_pyset(L, 1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_set1);
    }
    
    if (lua_istable(L, 2)) {
        py_set2 = lua_table_to_pyset(L, 2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1) Py_DECREF(py_set1);
        if (py_set2) Py_DECREF(py_set2);
        luaL_error(L, "Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_And(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);
    
    if (!result) {
        luaL_error(L, "Python set intersection failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

// 集合差集操作
int set_sub(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1)) || 
        !(lua_istable(L, 2) || is_pyset(L, 2))) {
        luaL_error(L, "Attempt to perform difference on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python集合进行差集操作
    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_set1 = lua_table_to_pyset(L, 1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_set1);
    }
    
    if (lua_istable(L, 2)) {
        py_set2 = lua_table_to_pyset(L, 2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1) Py_DECREF(py_set1);
        if (py_set2) Py_DECREF(py_set2);
        luaL_error(L, "Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_Subtract(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);
    
    if (!result) {
        luaL_error(L, "Python set difference failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

// 集合对称差集操作
int set_bxor(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pyset(L, 1)) || 
        !(lua_istable(L, 2) || is_pyset(L, 2))) {
        luaL_error(L, "Attempt to perform symmetric difference on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python集合进行对称差集操作
    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_set1 = lua_table_to_pyset(L, 1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_set1);
    }
    
    if (lua_istable(L, 2)) {
        py_set2 = lua_table_to_pyset(L, 2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1) Py_DECREF(py_set1);
        if (py_set2) Py_DECREF(py_set2);
        luaL_error(L, "Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_Xor(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);
    
    if (!result) {
        luaL_error(L, "Python set symmetric difference failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

int pushSetLua(lua_State* L, PyObject* set) {
    if (!PySet_Check(set)) {
        luaL_error(L, "Failed to set metatable for set");
        return 0;
    }
    void* userdata = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)userdata = set;
    Py_INCREF(set);
    lua_createtable(L, 0, 9);
    lua_pushcfunction(L, set_add);
    lua_setfield(L, -2, "__add");
    lua_pushcfunction(L, set_mul);
    lua_setfield(L, -2, "__mul");
    lua_pushcfunction(L, set_sub);
    lua_setfield(L, -2, "__sub");
    lua_pushcfunction(L, set_bxor);
    lua_setfield(L, -2, "__bxor");
    lua_pushcfunction(L, set_eq);
    lua_setfield(L, -2, "__eq");
    lua_pushcfunction(L, set_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, set_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, set_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertSetPython(lua_State* L, int index) {
    if (lua_istable(L, index)) {
        PyObject* py_set = PySet_New(convertListPython(L, index));
        return py_set;
    } else if (is_pyset(L, index)) {
        PyObject* py_set = *(PyObject**)lua_touserdata(L, index);
        Py_INCREF(py_set);
        return py_set;
    }
    luaL_error(L, "Attempt to convert a %s value to Python set", luaL_typename(L, index));
    return nullptr; // 不会到达这里
}