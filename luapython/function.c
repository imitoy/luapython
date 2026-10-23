#include "luapython.h"

#define isPythonFunction(L, index) (isPythonObject(L, index) && PyCallable_Check(*(PyObject**)lua_touserdata(L, index)))

/* Best effort callable name for error messages. Never raises. */
static void callable_name(PyObject* fn, char* buf, size_t size) {
    if (buf == NULL || size == 0) {
        return;
    }
    const char* attr = "__qualname__";
    PyObject* name = PyObject_GetAttrString(fn, attr);
    if (name == NULL) {
        PyErr_Clear();
        name = PyObject_GetAttrString(fn, "__name__");
        if (name == NULL) {
            PyErr_Clear();
            snprintf(buf, size, "<python callable>");
            return;
        }
    }
    size_t used = 0;
    buf[0] = '\0';
    PyObject* bytes = PyUnicode_AsEncodedString(name, "utf-8", "surrogateescape");
    Py_DECREF(name);
    if (bytes == NULL) {
        PyErr_Clear();
        snprintf(buf, size, "<python callable>");
        return;
    }
    const char* text = PyBytes_AsString(bytes);
    if (text != NULL) {
        used = (size_t)snprintf(buf, size, "%s", text);
        (void)used;
    }
    Py_DECREF(bytes);
}

/*
 * Called from luapython/tools.lua as: python_function(self, args, kwargs, n)
 *   1 = the callable, 2 = positional args table, 3 = kwargs table,
 *   4 = number of positional args.
 */
int function_call(lua_State* L) {
    if (!isPythonFunction(L, 1)) {
        return luaL_error(L, "function_call: attempt to call a %s value", luaL_typename(L, 1));
    }
    if (!lua_istable(L, 2) || !lua_istable(L, 3)) {
        return luaL_error(L, "function_call: internal error, argument tables expected");
    }
    lua_Integer nargs = luaL_checkinteger(L, 4);
    if (nargs < 0) {
        return luaL_error(L, "function_call: negative argument count");
    }

    PyObject* function = *(PyObject**)lua_touserdata(L, 1);
    PyObject* args = PyTuple_New(nargs);
    PyObject* kwargs = PyDict_New();
    if (args == NULL || kwargs == NULL) {
        Py_XDECREF(args);
        Py_XDECREF(kwargs);
        return luapython_throw(L, "function_call: out of memory");
    }

    for (lua_Integer i = 1; i <= nargs; i++) {
#if LUA_VERSION_NUM >= 503
        lua_geti(L, 2, i);
#else
        lua_rawgeti(L, 2, (int)i);
#endif
        PyObject* arg = convertPython(L, -1);
        lua_pop(L, 1);
        if (arg == NULL) {
            Py_DECREF(args);
            Py_DECREF(kwargs);
            return luapython_throw(L, "function_call: cannot convert argument");
        }
        PyTuple_SetItem(args, i - 1, arg);   /* steals the reference */
    }

    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char* key = lua_tostring(L, -2);
            PyObject* key_py = PyUnicode_FromString(key);
            PyObject* value = convertPython(L, -1);
            if (key_py == NULL || value == NULL) {
                Py_XDECREF(key_py);
                Py_XDECREF(value);
                lua_pop(L, 2);          /* value + key */
                Py_DECREF(args);
                Py_DECREF(kwargs);
                return luapython_throw(L, "function_call: cannot convert keyword argument");
            }
            int rc = PyDict_SetItem(kwargs, key_py, value);
            Py_DECREF(key_py);
            Py_DECREF(value);           /* PyDict_SetItem does NOT steal */
            if (rc != 0) {
                lua_pop(L, 2);
                Py_DECREF(args);
                Py_DECREF(kwargs);
                return luapython_throw(L, "function_call: cannot build keyword arguments");
            }
        }
        lua_pop(L, 1);
    }

    PyObject* result = PyObject_Call(function, args, kwargs);
    char name[256];
    if (result == NULL) {
        callable_name(function, name, sizeof name);
    }
    Py_DECREF(args);
    Py_DECREF(kwargs);
    if (result == NULL) {
        return luapython_throw(L, name);
    }

    /* A tuple result is expanded into multiple Lua return values (that is how
     * `local a, b = f()` gets several values). Note that this makes it
     * impossible to receive a real tuple from a call. */
    if (PyTuple_Check(result)) {
        Py_ssize_t size = PyTuple_Size(result);
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject* item = PyTuple_GetItem(result, i);   /* borrowed */
            Py_XINCREF(item);
            pushLua(L, item);                              /* consumes */
        }
        Py_DECREF(result);
        return (int)size;
    }
    pushLua(L, result);   /* consumes */
    return 1;
}

int pushFunctionLua(lua_State* L, PyObject* obj) {
    if (!PyCallable_Check(obj)) {
        return luaL_error(L, "pushFunctionLua: object is not callable");
    }
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_FUNCTION_NAME)) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_get_python_adapt_function);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            loadTools(L);
            lua_rawgeti(L, LUA_REGISTRYINDEX, tools_get_python_adapt_function);
        }
        lua_pushcfunction(L, function_call);
        lua_call(L, 1, 1);                 /* adapt(function_call) */
        lua_setfield(L, -2, "__call");
        lua_pushcfunction(L, python_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, python_index);
        lua_setfield(L, -2, "__index");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}
