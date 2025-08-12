#include "luapython.hpp"

int function_call(lua_State* L) {
    int nargs = lua_gettop(L)-1;
    if(!lua_isuserdata(L, 1)){
        luaL_error(L, "Stack top is not a function object");
        return 0;
    }
    PyObject* function = *(PyObject**)lua_touserdata(L, 1);
    if (!PyCallable_Check(function)) {
        luaL_error(L, "Attempt to call a %s object", Py_TYPE(function)->tp_name);
        return 0;
    }
    PyObject* args = PyTuple_New(nargs);
    if (!args) {
        luaL_error(L, "Failed to create argument tuple");
        return 0;
    }
    for (int i = 0; i < nargs; i++) {
        PyObject* arg = convertPython(L, i+2);
        if (!arg) {
            luaL_error(L, "Failed to convert argument %d", i+1);
            return 0;
        }
        PyTuple_SetItem(args, i, arg);
    }
    PyObject* result = PyObject_CallObject(function, args);
    pushLua(L, result);//here need to decref result possibly
    Py_DECREF(args);
    return 1;
}

int function_tostring(lua_State* L) {
    if (!lua_isuserdata(L, -1)) {
        luaL_error(L, "Attempt to convert a %s value to string", luaL_typename(L, -1));
        return 0;
    }
    PyObject* function = convertPython(L, -1);
    PyObject* str = PyObject_Str(function);
    lua_pushstring(L, PyUnicode_AsUTF8(str));
    Py_DECREF(str);
    return 1;
}

int pushFunctionLua(lua_State*L, PyObject* function){
    if(!PyCallable_Check(function)){
        luaL_error(L, "Function is not callable");
        return 0;
    }
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = function;
    Py_INCREF(function);
    lua_createtable(L, 0, 3);
    lua_pushcfunction(L, function_call);
    lua_setfield(L, -2, "__call");
    lua_pushcfunction(L, function_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
}