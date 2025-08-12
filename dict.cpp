#include "luapython.hpp"

// 检查是否为Python字典对象
static bool is_pydict(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) return false;
    
    // 获取元表
    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.dict");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2); // 弹出两个元表
        return is_same;
    }
    return false;
}

// 将Lua值转换为Python对象
static PyObject* lua_value_to_py(lua_State* L, int index) {
    if (lua_isstring(L, index)) {
        return PyUnicode_FromString(lua_tostring(L, index));
    } else if (lua_isnumber(L, index)) {
        return PyFloat_FromDouble(lua_tonumber(L, index));
    } else if (lua_isboolean(L, index)) {
        PyObject* result = lua_toboolean(L, index) ? Py_True : Py_False;
        Py_INCREF(result);
        return result;
    } else if (lua_isuserdata(L, index)) {
        PyObject* result = *(PyObject**)lua_touserdata(L, index);
        Py_INCREF(result);
        return result;
    } else if (lua_isnil(L, index)) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    return nullptr;
}

// 将Python对象转换为Lua值
static void py_to_lua_value(lua_State* L, PyObject* py_obj) {
    if (PyUnicode_Check(py_obj)) {
        const char* str = PyUnicode_AsUTF8(py_obj);
        if (str) lua_pushstring(L, str);
        else lua_pushnil(L);
    } else if (PyLong_Check(py_obj)) {
        long value = PyLong_AsLong(py_obj);
        lua_pushinteger(L, value);
    } else if (PyFloat_Check(py_obj)) {
        double value = PyFloat_AsDouble(py_obj);
        lua_pushnumber(L, value);
    } else if (py_obj == Py_True) {
        lua_pushboolean(L, true);
    } else if (py_obj == Py_False) {
        lua_pushboolean(L, false);
    } else if (py_obj == Py_None) {
        lua_pushnil(L);
    } else {
        // 对于其他类型，转换为字符串表示
        PyObject* str_repr = PyObject_Str(py_obj);
        if (str_repr) {
            const char* str = PyUnicode_AsUTF8(str_repr);
            if (str) lua_pushstring(L, str);
            else lua_pushnil(L);
            Py_DECREF(str_repr);
        } else {
            lua_pushnil(L);
        }
    }
}

// 将Lua表转换为Python字典
static PyObject* lua_table_to_pydict(lua_State* L, int index) {
    if (!lua_istable(L, index)) return nullptr;
    
    PyObject* py_dict = PyDict_New();
    if (!py_dict) return nullptr;
    
    lua_pushnil(L); // 第一个key
    while (lua_next(L, index - 1) != 0) {
        // key在-2，value在-1
        PyObject* py_key = lua_value_to_py(L, -2);
        PyObject* py_value = lua_value_to_py(L, -1);
        
        if (py_key && py_value) {
            int result = PyDict_SetItem(py_dict, py_key, py_value);
            if (result < 0) {
                Py_DECREF(py_key);
                Py_DECREF(py_value);
                Py_DECREF(py_dict);
                return nullptr;
            }
        }
        
        if (py_key) Py_DECREF(py_key);
        if (py_value) Py_DECREF(py_value);
        
        lua_pop(L, 1); // 弹出value，保留key用于下一次迭代
    }
    
    return py_dict;
}

// 将Python字典转换为Lua表
static int pydict_to_lua_table(lua_State* L, PyObject* py_dict) {
    if (!PyDict_Check(py_dict)) return 0;
    
    lua_newtable(L);
    PyObject* key, *value;
    Py_ssize_t pos = 0;
    
    while (PyDict_Next(py_dict, &pos, &key, &value)) {
        py_to_lua_value(L, key);
        py_to_lua_value(L, value);
        lua_settable(L, -3);
    }
    
    return 1;
}

// 字典长度
int dict_len(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1))) {
        luaL_error(L, "Attempt to get length of %s", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        lua_len(L, 1);
        return 1;
    }

    // Python字典长度
    PyObject* py_dict = *(PyObject**)lua_touserdata(L, 1);
    Py_ssize_t len = PyDict_Size(py_dict);
    lua_pushinteger(L, len);
    return 1;
}

// 字典相等比较
int dict_eq(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1)) || 
        !(lua_istable(L, 2) || is_pydict(L, 2))) {
        luaL_error(L, "Attempt to compare %s and %s as dictionaries", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python字典进行比较
    PyObject* py_dict1 = nullptr;
    PyObject* py_dict2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_dict1 = lua_table_to_pydict(L, 1);
    } else {
        py_dict1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_dict1);
    }
    
    if (lua_istable(L, 2)) {
        py_dict2 = lua_table_to_pydict(L, 2);
    } else {
        py_dict2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_dict2);
    }

    if (!py_dict1 || !py_dict2) {
        if (py_dict1) Py_DECREF(py_dict1);
        if (py_dict2) Py_DECREF(py_dict2);
        luaL_error(L, "Failed to create Python dictionaries");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_dict1, py_dict2, Py_EQ);
    Py_DECREF(py_dict1);
    Py_DECREF(py_dict2);
    
    lua_pushboolean(L, result);
    return 1;
}

// 字典索引访问
int dict_index(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1))) {
        luaL_error(L, "Attempt to index %s", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        // 对于Lua表，使用标准索引操作
        lua_pushvalue(L, 2); // 复制key
        lua_gettable(L, 1);
        return 1;
    }

    // Python字典索引访问
    PyObject* py_dict = *(PyObject**)lua_touserdata(L, 1);
    PyObject* py_key = lua_value_to_py(L, 2);
    
    if (!py_key) {
        luaL_error(L, "Invalid key type for dictionary access");
        return 0;
    }

    PyObject* py_value = PyDict_GetItem(py_dict, py_key);
    Py_DECREF(py_key);
    
    if (!py_value) {
        lua_pushnil(L);
        return 1;
    }

    py_to_lua_value(L, py_value);
    return 1;
}

// 字典索引赋值
int dict_newindex(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1))) {
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

    // Python字典索引赋值
    PyObject* py_dict = *(PyObject**)lua_touserdata(L, 1);
    PyObject* py_key = lua_value_to_py(L, 2);
    PyObject* py_value = lua_value_to_py(L, 3);
    
    if (!py_key || !py_value) {
        if (py_key) Py_DECREF(py_key);
        if (py_value) Py_DECREF(py_value);
        luaL_error(L, "Invalid key or value type for dictionary assignment");
        return 0;
    }

    int result = PyDict_SetItem(py_dict, py_key, py_value);
    Py_DECREF(py_key);
    Py_DECREF(py_value);
    
    if (result < 0) {
        luaL_error(L, "Failed to set dictionary item");
        return 0;
    }

    return 0;
}

// 字典转换为字符串
int dict_tostring(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1))) {
        luaL_error(L, "Attempt to convert a %s value to string", luaL_typename(L, 1));
        return 0;
    }

    if (lua_istable(L, 1)) {
        // 对于Lua表，返回表的字符串表示
        lua_pushstring(L, "table");
        return 1;
    }

    // Python字典转换为字符串
    PyObject* py_dict = *(PyObject**)lua_touserdata(L, 1);
    PyObject* str_repr = PyObject_Str(py_dict);
    if (!str_repr) {
        luaL_error(L, "Failed to convert Python dictionary to string");
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

// 字典合并操作
int dict_add(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1)) || 
        !(lua_istable(L, 2) || is_pydict(L, 2))) {
        luaL_error(L, "Attempt to merge %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python字典进行合并操作
    PyObject* py_dict1 = nullptr;
    PyObject* py_dict2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_dict1 = lua_table_to_pydict(L, 1);
    } else {
        py_dict1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_dict1);
    }
    
    if (lua_istable(L, 2)) {
        py_dict2 = lua_table_to_pydict(L, 2);
    } else {
        py_dict2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_dict2);
    }

    if (!py_dict1 || !py_dict2) {
        if (py_dict1) Py_DECREF(py_dict1);
        if (py_dict2) Py_DECREF(py_dict2);
        luaL_error(L, "Failed to create Python dictionaries");
        return 0;
    }

    // 创建新字典并合并
    PyObject* result = PyDict_Copy(py_dict1);
    if (!result) {
        Py_DECREF(py_dict1);
        Py_DECREF(py_dict2);
        luaL_error(L, "Failed to copy dictionary");
        return 0;
    }

    PyObject* key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(py_dict2, &pos, &key, &value)) {
        PyDict_SetItem(result, key, value);
    }

    Py_DECREF(py_dict1);
    Py_DECREF(py_dict2);

    lua_pushlightuserdata(L, result);
    pushDictLua(L, result);
    return 1;
}

// 字典交集操作（键的交集）
int dict_mul(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1)) || 
        !(lua_istable(L, 2) || is_pydict(L, 2))) {
        luaL_error(L, "Attempt to perform intersection on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python字典进行交集操作
    PyObject* py_dict1 = nullptr;
    PyObject* py_dict2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_dict1 = lua_table_to_pydict(L, 1);
    } else {
        py_dict1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_dict1);
    }
    
    if (lua_istable(L, 2)) {
        py_dict2 = lua_table_to_pydict(L, 2);
    } else {
        py_dict2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_dict2);
    }

    if (!py_dict1 || !py_dict2) {
        if (py_dict1) Py_DECREF(py_dict1);
        if (py_dict2) Py_DECREF(py_dict2);
        luaL_error(L, "Failed to create Python dictionaries");
        return 0;
    }

    // 创建新字典，包含键的交集
    PyObject* result = PyDict_New();
    if (!result) {
        Py_DECREF(py_dict1);
        Py_DECREF(py_dict2);
        luaL_error(L, "Failed to create new dictionary");
        return 0;
    }

    PyObject* key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(py_dict1, &pos, &key, &value)) {
        if (PyDict_Contains(py_dict2, key)) {
            PyDict_SetItem(result, key, value);
        }
    }

    Py_DECREF(py_dict1);
    Py_DECREF(py_dict2);

    lua_pushlightuserdata(L, result);
    pushDictLua(L, result);
    return 1;
}

// 字典差集操作（键的差集）
int dict_sub(lua_State* L) {
    if (!(lua_istable(L, 1) || is_pydict(L, 1)) || 
        !(lua_istable(L, 2) || is_pydict(L, 2))) {
        luaL_error(L, "Attempt to perform difference on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 转换为Python字典进行差集操作
    PyObject* py_dict1 = nullptr;
    PyObject* py_dict2 = nullptr;
    
    if (lua_istable(L, 1)) {
        py_dict1 = lua_table_to_pydict(L, 1);
    } else {
        py_dict1 = *(PyObject**)lua_touserdata(L, 1);
        Py_INCREF(py_dict1);
    }
    
    if (lua_istable(L, 2)) {
        py_dict2 = lua_table_to_pydict(L, 2);
    } else {
        py_dict2 = *(PyObject**)lua_touserdata(L, 2);
        Py_INCREF(py_dict2);
    }

    if (!py_dict1 || !py_dict2) {
        if (py_dict1) Py_DECREF(py_dict1);
        if (py_dict2) Py_DECREF(py_dict2);
        luaL_error(L, "Failed to create Python dictionaries");
        return 0;
    }

    // 创建新字典，包含键的差集
    PyObject* result = PyDict_New();
    if (!result) {
        Py_DECREF(py_dict1);
        Py_DECREF(py_dict2);
        luaL_error(L, "Failed to create new dictionary");
        return 0;
    }

    PyObject* key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(py_dict1, &pos, &key, &value)) {
        if (!PyDict_Contains(py_dict2, key)) {
            PyDict_SetItem(result, key, value);
        }
    }

    Py_DECREF(py_dict1);
    Py_DECREF(py_dict2);

    lua_pushlightuserdata(L, result);
    pushDictLua(L, result);
    return 1;
}

int pushDictLua(lua_State*L, PyObject* dict) {
    if(!PyDict_Check(dict)){
        luaL_error(L, "Not a dict");
        return 0;
    }
    void* userdata = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)userdata = dict;
    Py_INCREF(dict);
    lua_createtable(L, 0, 9);
    lua_pushcfunction(L, dict_len);
    lua_setfield(L, -2, "__len");
    lua_pushcfunction(L, dict_eq);
    lua_setfield(L, -2, "__eq");
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
    lua_pushcfunction(L, dict_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertDictPython(lua_State* L, int index) {
    if (lua_istable(L, index)) {
        PyObject* py_dict = PyDict_New();
        lua_pushvalue(L, index);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            // 检查键是否为字符串
            if (lua_isstring(L, -2)) {
                lua_pushvalue(L, -2);
                const char* key = lua_tostring(L, -1);
                PyObject* py_key = PyUnicode_FromString(key);
                PyObject* py_value = convertPython(L, -2);
                lua_pop(L, 1);
                if (py_key && py_value) {
                    PyDict_SetItem(py_dict, py_key, py_value);
                    //Py_DECREF(py_key);
                    //Py_DECREF(py_value);
                } else {
                    Py_XDECREF(py_key);
                    Py_XDECREF(py_value);
                    Py_DECREF(py_dict);
                    luaL_error(L, "Failed to convert Lua table to Python dictionary");
                    return nullptr;
                }
            }
            // 如果键不是字符串，跳过该键值对
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        return py_dict;
    } else if (is_pydict(L, index)) {
        PyObject* py_dict = *(PyObject**)lua_touserdata(L, index);
        Py_INCREF(py_dict);
        return py_dict;
    }
    luaL_error(L, "Attempt to convert a %s value to Python dictionary", luaL_typename(L, index));
    return nullptr; // 不会到达这里
}