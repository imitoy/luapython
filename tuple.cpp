#include "luapython.hpp"

static bool is_pytuple(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) return false;
    
    if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, "luapython.tuple");
        bool is_same = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        return is_same;
    }
    return false;
}

int tuple_len(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pytuple(L, -1))) {
        luaL_error(L, "tuple_len: Attempt to get length of %s", luaL_typename(L, -1));
        return 0;
    }

    if (lua_istable(L, -1)) {
        lua_len(L, -1);
        return 1;
    }

    PyObject* py_tuple = *(PyObject**)lua_touserdata(L, -1);
    Py_ssize_t len = PyTuple_Size(py_tuple);
    lua_pushinteger(L, len);
    return 1;
}

int tuple_eq(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pytuple(L, -1)) || 
        !(lua_istable(L, -2) || is_pytuple(L, -2))) {
        luaL_error(L, "tuple_eq: Attempt to compare %s and %s as tuples", 
                  luaL_typename(L, -2), luaL_typename(L, -1));
        return 0;
    }

    PyObject* py_tuple1 = nullptr;
    PyObject* py_tuple2 = nullptr;
    
    if (lua_istable(L, -1)) {
        py_tuple1 = convertPython(L, -1);
    } else {
        py_tuple1 = *(PyObject**)lua_touserdata(L, -1);
        Py_INCREF(py_tuple1);
    }
    
    if (lua_istable(L, -2)) {
        py_tuple2 = convertPython(L, -2);
    } else {
        py_tuple2 = *(PyObject**)lua_touserdata(L, -2);
        Py_INCREF(py_tuple2);
    }

    if (!py_tuple1 || !py_tuple2) {
        if (py_tuple1) Py_DECREF(py_tuple1);
        if (py_tuple2) Py_DECREF(py_tuple2);
        luaL_error(L, "tuple_eq: Failed to create Python tuples");
        return 0;
    }

    int result = PyObject_RichCompareBool(py_tuple1, py_tuple2, Py_EQ);
    Py_DECREF(py_tuple1);
    Py_DECREF(py_tuple2);
    
    lua_pushboolean(L, result);
    return 1;
}

int tuple_index(lua_State* L) {
    if (!(lua_istable(L, -2) || is_pytuple(L, -2))) {
        luaL_error(L, "tuple_index: Attempt to index %s", luaL_typename(L, -2));
        return 0;
    }

    if (lua_istable(L, -2)) {
        lua_geti(L, -2, luaL_checkinteger(L, -1));
        return 1;
    }

    PyObject* py_tuple = *(PyObject**)lua_touserdata(L, -2);
    long index = luaL_checkinteger(L, -1);
    Py_ssize_t py_index = index - 1;
    Py_ssize_t size = PyTuple_Size(py_tuple);
    
    if (py_index < 0 || py_index >= size) {
        lua_pushnil(L);
        return 1;
    }

    PyObject* py_value = PyTuple_GetItem(py_tuple, py_index);
    pushLua(L, py_value);
    return 1;
}

int tuple_tostring(lua_State* L) {
    if (!(lua_istable(L, -1) || is_pytuple(L, -1))) {
        luaL_error(L, "tuple_tostring: Attempt to convert a %s value to string", luaL_typename(L, -1));
        return 0;
    }

    if (lua_istable(L, -1)) {
        lua_pushstring(L, "table");
        return 1;
    }

    PyObject* py_tuple = *(PyObject**)lua_touserdata(L, -1);
    PyObject* str_repr = PyObject_Str(py_tuple);
    if (!str_repr) {
        luaL_error(L, "tuple_tostring: Failed to convert Python tuple to string");
        return 0;
    }
    
    const char* str = PyUnicode_AsUTF8(str_repr);
    if (!str) {
        Py_DECREF(str_repr);
        luaL_error(L, "tuple_tostring: Failed to get string representation");
        return 0;
    }
    
    lua_pushstring(L, str);
    Py_DECREF(str_repr);
    return 1;
}

int pushTupleLua(lua_State*L, PyObject* tuple) { 
    if(!PyTuple_Check(tuple)){
        luaL_error(L, "pushTupleLua: Not a tuple");
        return 0;
    }
    void* tuple_ptr = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)tuple_ptr = tuple;
    Py_INCREF(tuple);
    lua_createtable(L, 0, 6);
    lua_pushcfunction(L, tuple_len);
    lua_setfield(L, -2, "__len");
    lua_pushcfunction(L, tuple_eq);
    lua_setfield(L, -2, "__eq");
    lua_pushcfunction(L, tuple_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, tuple_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertTuplePython(lua_State* L, int index) {
    if (lua_istable(L, index)) {
        PyObject* py_list = convertListPython(L, index);
        PyObject* py_tuple = PyList_AsTuple(py_list);
        Py_DECREF(py_list);
        return py_tuple;
    } else if (is_pytuple(L, index)) {
        PyObject* py_tuple = *(PyObject**)lua_touserdata(L, index);
        Py_INCREF(py_tuple);
        return py_tuple;
    }
    luaL_error(L, "convertTuplePython: Attempt to convert a %s value to Python tuple", luaL_typename(L, index));
    return nullptr;
}

