#include "luapython.hpp"

int function_call(lua_State* L) {
    int nargs = lua_gettop(L) - 1;
    if (!lua_isuserdata(L, 1)) {
        luaL_error(L, "function_call: Stack top is not a function object");
        return 0;
    }
    PyObject* function = *(PyObject**)lua_touserdata(L, 1);
    if (!PyCallable_Check(function)) {
        luaL_error(L, "function_call: Attempt to call a %s object", Py_TYPE(function)->tp_name);
        return 0;
    }

    if (nargs == 1 && lua_istable(L, 2)) {
        lua_pushvalue(L, 2);
        lua_pushnil(L);
        bool p = true;
        PyObject* inspect = PyImport_ImportModule("inspect");
        PyObject* signature = PyObject_GetAttrString(inspect, "signature");
        PyObject* signature_of_function = PyObject_CallFunctionObjArgs(signature, function, NULL);
        PyObject* parameters = PyObject_GetAttrString(signature_of_function, "parameters");
        PyObject* keys = PyObject_CallMethod(parameters, "keys", NULL);
        while (lua_next(L, 3) != 0) {
            lua_pushvalue(L, -2);
            if (lua_isstring(L, -1)) {
                const char* key = lua_tostring(L, -1);
                PyObject* key_py = PyUnicode_FromString(key);
                if (!PySequence_Contains(keys, key_py)) {
                    lua_pop(L, 2);
                    Py_DECREF(key_py);
                    p = false;
                    break;
                }
                Py_DECREF(key_py);
            } else {
                lua_pop(L, 2);
                p = false;
                break;
            }
            lua_pop(L, 2);
        }
        Py_DECREF(parameters);
        Py_DECREF(keys);
        Py_DECREF(signature_of_function);
        Py_DECREF(signature);
        Py_DECREF(inspect);
        lua_pop(L, 1);
        if (p) {
            lua_pushvalue(L, 2);
            lua_pushnil(L);
            lua_Integer len = lua_rawlen(L, 3);
            PyObject* args = PyTuple_New(len);
            PyObject* kwargs = PyDict_New();
            while (lua_next(L, 3) != 0) {
                lua_pushvalue(L, -2);
                if (lua_type(L, -1) == LUA_TSTRING) {
                    const char* key = lua_tostring(L, -1);
                    lua_pushvalue(L, -2);
                    PyObject* key_py = PyUnicode_FromString(key);
                    PyObject* value = convertPython(L, -1);
                    PyDict_SetItem(kwargs, key_py, value);
                    Py_DECREF(key_py);
                    Py_DECREF(value);
                    lua_pop(L, 1);
                }
                lua_pop(L, 2);
            }
            for (int i = 1; i <= len; i++) {
                lua_geti(L, 3, i);
                lua_pushvalue(L, -1);
                PyObject* arg = convertPython(L, -1);
                lua_pop(L, 2);
                if (!arg) {
                    luaL_error(L, "function_call: Failed to convert argument %d", i + 1);
                    return 0;
                }
                PyTuple_SetItem(args, i, arg);
            }
            lua_pop(L, 1);
            PyObject* result = PyObject_Call(function, args, kwargs);
            pushLua(L, result); // here need to decref result possibly
            Py_DECREF(args);
            return 1;
        }
    }

    PyObject* args = PyTuple_New(nargs);
    if (!args) {
        luaL_error(L, "function_call: Failed to create argument tuple");
        return 0;
    }
    for (int i = 0; i < nargs; i++) {
        PyObject* arg = convertPython(L, i + 2);
        if (!arg) {
            luaL_error(L, "function_call: Failed to convert argument %d", i + 1);
            return 0;
        }
        PyTuple_SetItem(args, i, arg);
    }
    PyObject* result = PyObject_CallObject(function, args);
    pushLua(L, result); // here need to decref result possibly
    Py_DECREF(args);
    return 1;
}

int function_tostring(lua_State* L) {
    if (!lua_isuserdata(L, -1)) {
        luaL_error(L, "function_tostring: Attempt to convert a %s value to string", luaL_typename(L, -1));
        return 0;
    }
    PyObject* function = convertPython(L, -1);
    PyObject* str = PyObject_Str(function);
    lua_pushstring(L, PyUnicode_AsUTF8(str));
    Py_DECREF(str);
    return 1;
}

int pushFunctionLua(lua_State* L, PyObject* function) {
    if (!PyCallable_Check(function)) {
        luaL_error(L, "pushFunctionLua: Function is not callable");
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