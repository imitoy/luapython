#include "luapython.h"

#define isPythonFunction(L, index) (isPythonObject(L, index) && PyCallable_Check(*(PyObject**)lua_touserdata(L, index)))

int function_call(lua_State* L) {
    if (!lua_isnumber(L, -1)) {
        luaL_error(L, "function_call: The last argument must be the number of arguments");
        return 0;
    }
    int nargs = lua_tonumber(L, -1);
    int python_func_index = -2 - nargs;
    if (!isPythonFunction(L, python_func_index)) {
        luaL_error(L, "function_call: Attempt to call a %s object", luaL_typename(L, python_func_index));
        return 0;
    }
    PyObject* function = *(PyObject**)lua_touserdata(L, python_func_index);
    if (!PyCallable_Check(function)) {
        luaL_error(L, "function_call: Attempt to call a %s object", Py_TYPE(function)->tp_name);
        return 0;
    }
    if (nargs == 1 && lua_istable(L, -2)) {
        lua_pushvalue(L, -2);
        lua_pushnil(L);
        bool p = true;
        PyObject* inspect = PyImport_ImportModule("inspect");
        PyObject* signature = PyObject_GetAttrString(inspect, "signature");
        PyObject* signature_of_function = PyObject_CallFunctionObjArgs(signature, function, NULL);
        if (PyErr_Occurred()) {
            PyErr_Clear();
            Py_XDECREF(signature_of_function);
            Py_XDECREF(signature);
            Py_XDECREF(inspect);
            goto normal;
        }
        PyObject* parameters = PyObject_GetAttrString(signature_of_function, "parameters");
        PyObject* keys = PyObject_CallMethod(parameters, "keys", NULL);
        if (PyErr_Occurred()) {
            PyErr_Print();
            Py_XDECREF(parameters);
            Py_XDECREF(keys);
            Py_XDECREF(signature_of_function);
            Py_XDECREF(signature);
            Py_XDECREF(inspect);
            luaL_error(L, "function_call: Failed to import inspect module");
            return 0;
        }
        while (lua_next(L, -2) != 0) {
            lua_pushvalue(L, -2);
            if (lua_isstring(L, -1)) {
                const char* key = lua_tostring(L, -1);
                PyObject* key_py = PyUnicode_FromString(key);
                if (!PySequence_Contains(keys, key_py)) {
                    lua_pop(L, 2);
                    Py_XDECREF(key_py);
                    p = false;
                    break;
                }
                Py_XDECREF(key_py);
            } else {
                lua_pop(L, 2);
                p = false;
                break;
            }
            lua_pop(L, 2);
        }
        Py_XDECREF(parameters);
        Py_XDECREF(keys);
        Py_XDECREF(signature_of_function);
        Py_XDECREF(signature);
        Py_XDECREF(inspect);
        lua_pop(L, 1);
        if (p) {
            lua_pushvalue(L, -2);
            lua_pushnil(L);
            lua_Integer len = lua_rawlen(L, -2);
            PyObject* args = PyTuple_New(len);
            PyObject* kwargs = PyDict_New();
            while (lua_next(L, -2) != 0) {
                lua_pushvalue(L, -2);
                if (lua_type(L, -1) == LUA_TSTRING) {
                    const char* key = lua_tostring(L, -1);
                    lua_pushvalue(L, -2);
                    PyObject* key_py = PyUnicode_FromString(key);
                    PyObject* value = convertPython(L, -1);
                    PyDict_SetItem(kwargs, key_py, value);
                    Py_XDECREF(key_py);
                    Py_XDECREF(value);
                    lua_pop(L, 1);
                }
                lua_pop(L, 2);
            }
            for (int i = 1; i <= len; i++) {
                lua_geti(L, -1, i);
                lua_pushvalue(L, -1);
                PyObject* arg = convertPython(L, -1);
                lua_pop(L, 2);
                if (!arg) {
                    luaL_error(L, "function_call: Failed to convert argument %d", i + 1);
                    return 0;
                }
                PyTuple_SetItem(args, i, arg);
                Py_XDECREF(arg);
            }
            lua_pop(L, 1);
            PyObject* result = PyObject_Call(function, args, kwargs);
            if (PyErr_Occurred()) {
                PyErr_Print();
                luaL_error(L, "function_call: Error calling function");
                Py_XDECREF(args);
                Py_XDECREF(kwargs);
                return 0;
            }
            pushLua(L, result);
            Py_XDECREF(args);
            Py_XDECREF(result);
            return 1;
        }
    }

normal:
    PyObject* args = PyTuple_New(nargs);
    if (!args) {
        luaL_error(L, "function_call: Failed to create argument tuple");
        return 0;
    }
    for (int i = 0; i < nargs; i++) {
        PyObject* arg = convertPython(L, -2 - i);
        if (!arg) {
            luaL_error(L, "function_call: Failed to convert argument %d", i + 1);
            return 0;
        }
        PyTuple_SetItem(args, i, arg);
        Py_XDECREF(arg);
    }
    PyObject* result = PyObject_CallObject(function, args);
    pushLua(L, result);
    Py_XDECREF(args);
    Py_XDECREF(result);
    return 1;
}

int table_function_index = 0;

int pushFunctionLua(lua_State* L, PyObject* obj) {
    if (!PyCallable_Check(obj)) {
        luaL_error(L, "pushFunctionLua: Function is not callable");
        return 0;
    }
    if (table_function_index != 0) {
        void* point = lua_newuserdata(L, sizeof(PyObject*));
        *(PyObject**)point = obj;
        Py_XINCREF(obj);
        lua_rawgeti(L, LUA_REGISTRYINDEX, table_function_index);
        if (!lua_istable(L, -1)) {
            luaL_error(L, "pushFunctionLua: Internal error, class index is not a table");
            return 0;
        }
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_createtable(L, 0, 4);
    const char prefix[] = PREFIX;
    const char name[] = "/local/lib/lua/5.4/luapython/python_function.lua";
    char path[strlen(prefix) + strlen(name) + 1];
    strcpy((char*)path, prefix);
    strcat((char*)path, name);
    luaL_loadfile(L, path);
    lua_pushcfunction(L, function_call);
    lua_call(L, 1, 1);
    lua_setfield(L, -2, "__call");
    lua_pushcfunction(L, python_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, python_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushstring(L, PYTHON_OBJECT_NAME);
    lua_setfield(L, -2, "__name");
    table_function_index = luaL_ref(L, LUA_REGISTRYINDEX);
    return pushFunctionLua(L, obj);
}