#include "luapython.hpp"

// 检查是否为Python集合对象
static bool is_pyset(lua_State* L, int index) {
    if (!lua_isuserdata(L, index))
        return false;

    // 获取元表
    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.set");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2); // 弹出两个元表
        return is_same;
    }
    return false;
}

int set_len(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pyset(L, -1))) {
        luaL_error(L, "set_len: Attempt to get length of %s", luaL_typename(L, -1));
        return 0;
    }

    if (lua_istable(L, -1)) {
        lua_len(L, -1);
        return 1;
    }

    PyObject* py_set = *(PyObject**)lua_touserdata(L, -1);
    Py_ssize_t len = PySet_Size(py_set);
    lua_pushinteger(L, len);
    return 1;
}

int set_eq(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pyset(L, -1)) || !(lua_istable(L, -2) || is_pyset(L, -2))) {
        luaL_error(L, "set_eq: Attempt to compare %s and %s as sets", luaL_typename(L, -2), luaL_typename(L, -1));
        return 0;
    }

    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;

    if (lua_istable(L, -2)) {
        py_set1 = convertPython(L, -2);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_set1);
    }

    if (lua_istable(L, -1)) {
        py_set2 = convertPython(L, -1);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1)
            Py_DECREF(py_set1);
        if (py_set2)
            Py_DECREF(py_set2);
        luaL_error(L, "set_eq: Failed to create Python sets");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_set1, py_set2, Py_EQ);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);

    lua_pushboolean(L, result);
    return 1;
}

int set_index(lua_State* L) {
    if (!(lua_istable(L, -2) || is_pyset(L, -2))) {
        luaL_error(L, "set_index: Attempt to index %s", luaL_typename(L, -2));
        return 0;
    }

    if (lua_istable(L, -2)) {
        lua_pushvalue(L, -1);
        lua_gettable(L, -3);
        return 1;
    }

    luaL_error(L, "set_index: Set objects do not support indexing");
    return 0;
}

int set_newindex(lua_State* L) {
    if (!(lua_istable(L, -3) || is_pyset(L, -3))) {
        luaL_error(L, "set_newindex: Attempt to assign to %s", luaL_typename(L, -3));
        return 0;
    }

    // if there is a table, we are not at the side

    /*if (lua_istable(L, -3)) {
        lua_pushvalue(L, 2); // key
        lua_pushvalue(L, 3); // value
        lua_settable(L, 1);
        return 0;
    }*/

    luaL_error(L, "set_newindex: Set objects do not support assignment");
    return 0;
}

int set_tostring(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pyset(L, -1))) {
        luaL_error(L, "set_tostring: Attempt to convert a %s value to string", luaL_typename(L, -1));
        return 0;
    }

    if (lua_istable(L, -1)) {
        lua_pushstring(L, "table");
        return 1;
    }

    // Python集合转换为字符串
    PyObject* py_set = *(PyObject**)lua_touserdata(L, -1);
    PyObject* str_repr = PyObject_Str(py_set);
    if (!str_repr) {
        luaL_error(L, "set_tostring: Failed to convert Python set to string");
        return 0;
    }

    const char* str = PyUnicode_AsUTF8(str_repr);
    if (!str) {
        Py_DECREF(str_repr);
        luaL_error(L, "set_tostring: Failed to get string representation");
        return 0;
    }

    lua_pushstring(L, str);
    Py_DECREF(str_repr);
    return 1;
}

int set_add(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pyset(L, -1)) || !(lua_istable(L, -2) || is_pyset(L, -2))) {
        luaL_error(L, "set_add: Attempt to perform union on %s and %s", luaL_typename(L, -2), luaL_typename(L, -1));
        return 0;
    }

    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;

    if (lua_istable(L, -1)) {
        py_set1 = convertPython(L, -1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_set1);
    }

    if (lua_istable(L, -2)) {
        py_set2 = convertPython(L, -2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1)
            Py_DECREF(py_set1);
        if (py_set2)
            Py_DECREF(py_set2);
        luaL_error(L, "set_add: Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_Or(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);

    if (!result) {
        luaL_error(L, "set_add: Python set union failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

int set_mul(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pyset(L, -1)) || !(lua_istable(L, -2) || is_pyset(L, -2))) {
        luaL_error(L, "set_mul: Attempt to perform intersection on %s and %s", luaL_typename(L, -2),
                   luaL_typename(L, -1));
        return 0;
    }

    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;

    if (lua_istable(L, -1)) {
        py_set1 = convertPython(L, -1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_set1);
    }

    if (lua_istable(L, -2)) {
        py_set2 = convertPython(L, -2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1)
            Py_DECREF(py_set1);
        if (py_set2)
            Py_DECREF(py_set2);
        luaL_error(L, "set_mul: Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_And(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);

    if (!result) {
        luaL_error(L, "set_mul: Python set intersection failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

int set_sub(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pyset(L, -1)) || !(lua_istable(L, -2) || is_pyset(L, -2))) {
        luaL_error(L, "set_sub: Attempt to perform difference on %s and %s", luaL_typename(L, -2),
                   luaL_typename(L, -1));
        return 0;
    }

    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;

    if (lua_istable(L, -1)) {
        py_set1 = convertPython(L, -1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_set1);
    }

    if (lua_istable(L, -2)) {
        py_set2 = convertPython(L, -2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1)
            Py_DECREF(py_set1);
        if (py_set2)
            Py_DECREF(py_set2);
        luaL_error(L, "set_sub: Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_Subtract(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);

    if (!result) {
        luaL_error(L, "set_sub: Python set difference failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

int set_bxor(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pyset(L, -1)) || !(lua_istable(L, -2) || is_pyset(L, -2))) {
        luaL_error(L, "set_bxor: Attempt to perform symmetric difference on %s and %s", luaL_typename(L, -2),
                   luaL_typename(L, -1));
        return 0;
    }

    PyObject* py_set1 = nullptr;
    PyObject* py_set2 = nullptr;

    if (lua_istable(L, -1)) {
        py_set1 = convertPython(L, -1);
    } else {
        py_set1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_set1);
    }

    if (lua_istable(L, -2)) {
        py_set2 = convertPython(L, -2);
    } else {
        py_set2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_set2);
    }

    if (!py_set1 || !py_set2) {
        if (py_set1)
            Py_DECREF(py_set1);
        if (py_set2)
            Py_DECREF(py_set2);
        luaL_error(L, "set_bxor: Failed to create Python sets");
        return 0;
    }

    PyObject* result = PyNumber_Xor(py_set1, py_set2);
    Py_DECREF(py_set1);
    Py_DECREF(py_set2);

    if (!result) {
        luaL_error(L, "set_bxor: Python set symmetric difference failed");
        return 0;
    }

    pushSetLua(L, result);
    return 1;
}

int pushSetLua(lua_State* L, PyObject* set) {
    if (!PySet_Check(set)) {
        luaL_error(L, "pushSetLua: Failed to set metatable for set");
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
    luaL_error(L, "convertSetPython: Attempt to convert a %s value to Python set", luaL_typename(L, index));
    return nullptr;
}