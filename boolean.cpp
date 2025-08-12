#include "luapython.hpp"

// 检查是否为Python布尔对象
static bool is_pyboolean(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) return false;
    
    // 获取元表
    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.boolean");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2); // 弹出两个元表
        return is_same;
    }
    return false;
}

// 获取布尔值（支持Lua布尔和Python布尔对象）
static bool get_boolean_value(lua_State* L, int index) {
    if (lua_isboolean(L, index)) {
        return lua_toboolean(L, index);
    } else if (is_pyboolean(L, index)) {
        PyObject* py_bool = *(PyObject**)lua_touserdata(L, index);
        return py_bool == Py_True;
    }
    return false; // 默认值，实际不会执行到这里
}

// 布尔值相等比较
int boolean_eq(lua_State* L) {
    // 检查两个操作数是否都是布尔类型（Lua布尔或Python布尔对象）
    if (!(lua_isboolean(L, 1) || is_pyboolean(L, 1)) && 
        !(lua_isboolean(L, 2) || is_pyboolean(L, 2))) {
        luaL_error(L, "Attempt to compare %s and %s as booleans", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    // 获取操作数的布尔值
    bool a = get_boolean_value(L, 1);
    bool b = get_boolean_value(L, 2);
    
    // 比较两个布尔值并返回结果
    lua_pushboolean(L, a == b);
    return 1;
}

// 布尔值转换为字符串
int boolean_tostring(lua_State* L) {
    // 检查操作数是否是布尔类型
    if (!(lua_isboolean(L, 1) || is_pyboolean(L, 1))) {
        luaL_error(L, "Attempt to convert a %s value to string", luaL_typename(L, 1));
        return 0;
    }

    // 获取布尔值并转换为字符串
    bool value = get_boolean_value(L, 1);
    lua_pushstring(L, value ? "true" : "false");
    return 1;
}

// 布尔值逻辑与
int boolean_and(lua_State* L) {
    if (!(lua_isboolean(L, 1) || is_pyboolean(L, 1)) || 
        !(lua_isboolean(L, 2) || is_pyboolean(L, 2))) {
        luaL_error(L, "Attempt to perform AND on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    bool a = get_boolean_value(L, 1);
    bool b = get_boolean_value(L, 2);
    lua_pushboolean(L, a && b);
    return 1;
}

// 布尔值逻辑或
int boolean_or(lua_State* L) {
    if (!(lua_isboolean(L, 1) || is_pyboolean(L, 1)) || 
        !(lua_isboolean(L, 2) || is_pyboolean(L, 2))) {
        luaL_error(L, "Attempt to perform OR on %s and %s", 
                  luaL_typename(L, 1), luaL_typename(L, 2));
        return 0;
    }

    bool a = get_boolean_value(L, 1);
    bool b = get_boolean_value(L, 2);
    lua_pushboolean(L, a || b);
    return 1;
}

// 布尔值逻辑非
int boolean_not(lua_State* L) {
    if (!(lua_isboolean(L, 1) || is_pyboolean(L, 1))) {
        luaL_error(L, "Attempt to negate a %s value", luaL_typename(L, 1));
        return 0;
    }

    bool value = get_boolean_value(L, 1);
    lua_pushboolean(L, !value);
    return 1;
}

int pushBooleanLua(lua_State* L, PyObject* boolean) {
    if(!PyBool_Check(boolean)) {
        luaL_error(L, "Failed to set metatable for boolean");
        return 0;
    }
    lua_pushboolean(L, PyObject_IsTrue(boolean));
    return 1;
}

PyObject* convertBoolean(lua_State* L, int index) {
    if (lua_isboolean(L, index)) {
        return lua_toboolean(L, index) ? Py_True : Py_False;
    } else if (is_pyboolean(L, index)) {
        PyObject* py_bool = *(PyObject**)lua_touserdata(L, index);
        Py_INCREF(py_bool);
        return py_bool;
    }
    luaL_error(L, "Attempt to convert a %s value to Python", luaL_typename(L, index));
    return nullptr; // 如果不是布尔类型，返回nullptr
}