#include "luapython.h"

/* ------------------------------------------------------------------------- */
/* diagnostics                                                               */
/* ------------------------------------------------------------------------- */

static void msg_appendf(char* buf, size_t size, size_t* used, const char* fmt, ...) {
    va_list ap;
    if (*used >= size) {
        return;
    }
    va_start(ap, fmt);
    int n = vsnprintf(buf + *used, size - *used, fmt, ap);
    va_end(ap);
    if (n > 0) {
        size_t add = (size_t)n;
        if (add >= size - *used) {
            add = size - *used - 1;
        }
        *used += add;
    }
}

static void msg_append_unicode(char* buf, size_t size, size_t* used, PyObject* str) {
    if (str == NULL) {
        return;
    }
    PyObject* bytes = PyUnicode_AsEncodedString(str, "utf-8", "surrogateescape");
    if (bytes == NULL) {
        PyErr_Clear();
        return;
    }
    const char* text = PyBytes_AsString(bytes);
    if (text != NULL) {
        msg_appendf(buf, size, used, "%s", text);
    }
    Py_DECREF(bytes);
}

void luapython_type_name(PyObject* obj, char* buf, size_t size) {
    if (buf == NULL || size == 0) {
        return;
    }
    buf[0] = '\0';
    if (obj == NULL) {
        return;
    }
    PyObject* name = PyObject_GetAttrString((PyObject*)Py_TYPE(obj), "__name__");
    if (name == NULL) {
        PyErr_Clear();
        return;
    }
    size_t used = 0;
    msg_append_unicode(buf, size, &used, name);
    Py_DECREF(name);
}

int luapython_throw(lua_State* L, const char* context) {
    char buf[1024];
    size_t used = 0;

    buf[0] = '\0';
    if (context != NULL) {
        msg_appendf(buf, sizeof buf, &used, "%s: ", context);
    }

    if (!PyErr_Occurred()) {
        msg_appendf(buf, sizeof buf, &used, "Python operation failed");
    } else {
        PyObject *type = NULL, *value = NULL, *tb = NULL;
        PyErr_Fetch(&type, &value, &tb);
        PyErr_NormalizeException(&type, &value, &tb);

        if (type != NULL) {
            PyObject* name = PyObject_GetAttrString(type, "__name__");
            if (name != NULL) {
                msg_append_unicode(buf, sizeof buf, &used, name);
                Py_DECREF(name);
            } else {
                PyErr_Clear();
            }
        }
        if (value != NULL) {
            PyObject* text = PyObject_Str(value);
            if (text != NULL) {
                msg_appendf(buf, sizeof buf, &used, ": ");
                msg_append_unicode(buf, sizeof buf, &used, text);
                Py_DECREF(text);
            } else {
                PyErr_Clear();
            }
        }
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(tb);
        /* Whatever happened above, the interpreter must be left clean: a
         * pending exception would silently degrade the next conversion. */
        PyErr_Clear();
    }

    return luaL_error(L, "%s", buf);
}

/* ------------------------------------------------------------------------- */
/* metatable bookkeeping                                                     */
/* ------------------------------------------------------------------------- */

/* Push the per-state registry table `name`, creating it on first use. */
static void push_state_table(lua_State* L, const char* name) {
    lua_getfield(L, LUA_REGISTRYINDEX, name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_createtable(L, 0, 16);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, name);
    }
}

void luapython_register_metatable(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return;
    }
    lua_pushvalue(L, -1);                          /* mt, mt */
    push_state_table(L, LUAPYTHON_METATABLES);     /* mt, mt, set */
    lua_pushvalue(L, -2);                          /* mt, mt, set, mt */
    lua_pushboolean(L, 1);
    lua_rawset(L, -3);                             /* mt, mt, set */
    lua_pop(L, 2);                                 /* mt */
}

/* Object check by metatable identity: unlike comparing __name strings this
 * cannot be forged from Lua (debug.setmetatable) because the registry set is
 * only reachable through the raw registry index. */
int isPythonObject(lua_State* L, int index) {
    int result = 0;

    if (lua_type(L, index) != LUA_TUSERDATA) {
        return 0;
    }
    if (!Py_IsInitialized()) {
        /* The interpreter is gone: every handle we own points into freed
         * memory, so refuse to touch it instead of crashing. */
        return 0;
    }
    if (lua_getmetatable(L, index) == 0) {
        return 0;
    }
    push_state_table(L, LUAPYTHON_METATABLES);     /* mt, set */
    lua_pushvalue(L, -2);                          /* mt, set, mt */
    lua_rawget(L, -2);                             /* mt, set, value */
    result = lua_toboolean(L, -1);
    lua_pop(L, 3);                                 /* - */
    return result;
}

/* ------------------------------------------------------------------------- */
/* module level functions                                                    */
/* ------------------------------------------------------------------------- */

static int python_initialize(lua_State* L) {
    if (Py_IsInitialized()) {
        return luaL_error(L,
            "python_initialize: the Python interpreter is already running "
            "(started by the host program or by an earlier luapython.load())");
    }
    Py_Initialize();
    if (!Py_IsInitialized()) {
        return luaL_error(L, "python_initialize: Py_Initialize() failed");
    }
    return 0;
}

static int python_finalize(lua_State* L) {
    if (!Py_IsInitialized()) {
        return luaL_error(L, "python_finalize: the Python interpreter is not running");
    }
    /* Every handle handed to Lua becomes dangling here. isPythonObject()
     * reports 0 and python_gc() becomes a no-op once Py_Finalize() has run,
     * so surviving objects degrade into inert userdata instead of
     * dereferencing freed memory. */
    Py_Finalize();
    return 0;
}

/* Called (through luapython/import.lua) as (module_name, verbose).
 * Returns nil - not an error - when the module cannot be imported, because
 * import.lua relies on that to retry with its dotted-name fallback. */
static int python_import(lua_State* L) {
    if (!Py_IsInitialized()) {
        return luaL_error(L, "luapython has not been loaded; call luapython.load() first");
    }
    const char* module_name = lua_tostring(L, 1);
    if (module_name == NULL) {
        return luaL_error(L, "luapython.import: string expected, got %s", luaL_typename(L, 1));
    }
    int verbose = lua_toboolean(L, 2);

    PyObject* name = PyUnicode_FromString(module_name);
    if (name == NULL) {
        return luapython_throw(L, "luapython.import");
    }
    PyObject* module = PyImport_Import(name);
    Py_DECREF(name);
    if (module == NULL) {
        if (verbose) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        lua_pushnil(L);
        return 1;
    }
    pushLua(L, module);   /* consumes the reference */
    return 1;
}

static int python_set(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python set", luaL_typename(L, -1));
    }
    PyObject* set = convertSetPython(L, -1);
    if (set == NULL) {
        return luapython_throw(L, "luapython.set");
    }
    pushSetLua(L, set);
    return 1;
}

static int python_dict(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python dict", luaL_typename(L, -1));
    }
    PyObject* dict = convertDictPython(L, -1);
    if (dict == NULL) {
        return luapython_throw(L, "luapython.dict");
    }
    pushDictLua(L, dict);
    return 1;
}

static int python_tuple(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python tuple", luaL_typename(L, -1));
    }
    PyObject* tuple = convertTuplePython(L, -1);
    if (tuple == NULL) {
        return luapython_throw(L, "luapython.tuple");
    }
    pushTupleLua(L, tuple);
    return 1;
}

static int python_list(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python list", luaL_typename(L, -1));
    }
    PyObject* list = convertListPython(L, -1);
    if (list == NULL) {
        return luapython_throw(L, "luapython.list");
    }
    pushListLua(L, list);
    return 1;
}

int python_tostring(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        return luaL_error(L, "python_tostring: not a Python object");
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    if (obj == NULL) {
        lua_pushliteral(L, "<finalized python object>");
        return 1;
    }
    if (obj == Py_None) {
        lua_pushliteral(L, "None");
        return 1;
    }
    PyObject* str = PyObject_Str(obj);
    if (str == NULL) {
        return luapython_throw(L, "python_tostring");
    }
    pushStringLua(L, str);   /* consumes */
    return 1;
}

int python_gc(lua_State* L) {
    if (lua_type(L, -1) != LUA_TUSERDATA) {
        return 0;
    }
    if (!Py_IsInitialized()) {
        /* Interpreter already finalized: the object is freed memory. Leaking
         * here is the only safe option, and the process is going away. */
        return 0;
    }
    PyObject** slot = (PyObject**)lua_touserdata(L, -1);
    if (slot == NULL || *slot == NULL) {
        return 0;
    }
    PyObject* obj = *slot;
    *slot = NULL;            /* a second __gc must not double free */
    Py_XDECREF(obj);
    return 0;
}

/* lua_tostring() happily converts numbers but returns NULL for tables,
 * booleans and nil - which used to be handed straight to CPython. */
static const char* key_from_lua(lua_State* L, int index) {
    int type = lua_type(L, index);
    if (type != LUA_TSTRING && type != LUA_TNUMBER) {
        return NULL;
    }
    return lua_tostring(L, index);
}

int python_index(lua_State* L) {
    if (!isPythonObject(L, -2)) {
        return luaL_error(L, "python_index: attempt to index a %s value", luaL_typename(L, -2));
    }
    const char* key = key_from_lua(L, -1);
    if (key == NULL) {
        return luaL_error(L, "python_index: attribute name must be a string, got %s",
                          luaL_typename(L, -1));
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -2);
    if (obj == NULL) {
        return luaL_error(L, "python_index: the Python object is no longer valid");
    }
    PyObject* attr = PyObject_GetAttrString(obj, key);
    if (attr == NULL) {
        /* Same contract as Python's getattr(obj, name, None): a missing
         * attribute yields nil, but an exception raised *inside* a property
         * is a real error and must not be swallowed. */
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            lua_pushnil(L);
            return 1;
        }
        return luapython_throw(L, key);
    }
    pushLua(L, attr);   /* consumes */
    return 1;
}

int python_newindex(lua_State* L) {
    if (!isPythonObject(L, -3)) {
        return luaL_error(L, "python_newindex: attempt to index a %s value", luaL_typename(L, -3));
    }
    const char* key = key_from_lua(L, -2);
    if (key == NULL) {
        return luaL_error(L, "python_newindex: attribute name must be a string, got %s",
                          luaL_typename(L, -2));
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -3);
    if (obj == NULL) {
        return luaL_error(L, "python_newindex: the Python object is no longer valid");
    }
    PyObject* value = convertPython(L, -1);
    if (value == NULL) {
        return luapython_throw(L, "python_newindex: cannot convert value");
    }
    int result = PyObject_SetAttrString(obj, key, value);
    Py_DECREF(value);
    if (result != 0) {
        return luapython_throw(L, key);
    }
    return 0;
}

int python_addr(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        return luaL_error(L, "python_addr: not a Python object");
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    lua_pushfstring(L, "%p", obj);
    return 1;
}

int pushLua(lua_State* L, PyObject* obj) {
    if (obj == NULL || obj == Py_None) {
        /* None/True/False are singletons: no DECREF needed, and callers may
         * pass them without owning a reference. */
        lua_pushnil(L);
        return 1;
    }
    if (PyBool_Check(obj)) {
        lua_pushboolean(L, obj == Py_True);
        return 1;
    }
    if (PyNumber_Check(obj)) {
        return pushNumberLua(L, obj);
    }
    if (PyUnicode_Check(obj)) {
        return pushStringLua(L, obj);
    }
    if (PySet_Check(obj)) {
        return pushSetLua(L, obj);
    }
    if (PyDict_Check(obj)) {
        return pushDictLua(L, obj);
    }
    if (PyTuple_Check(obj)) {
        return pushTupleLua(L, obj);
    }
    if (PyList_Check(obj)) {
        return pushListLua(L, obj);
    }
    if (PyModule_Check(obj)) {
        return pushModuleLua(L, obj);
    }
    if (PyIter_Check(obj)) {
        return pushIterLua(L, obj);
    }
    if (PyCallable_Check(obj)) {
        return pushFunctionLua(L, obj);
    }
    return pushClassLua(L, obj);
}

PyObject* luapython_number_to_py(lua_State* L, int index) {
    if (lua_type(L, index) != LUA_TNUMBER) {
        return NULL;
    }
#if LUA_VERSION_NUM >= 503
    if (lua_isinteger(L, index)) {
        return PyLong_FromLongLong((long long)lua_tointeger(L, index));
    }
#endif
    double value = lua_tonumber(L, index);
    /* Range check first: casting an out-of-range double to long long is
     * undefined behaviour, so it must never be evaluated unconditionally. */
    if (value >= -9223372036854775808.0 && value < 9223372036854775808.0 &&
        value == (double)(long long)value) {
        return PyLong_FromLongLong((long long)value);
    }
    return PyFloat_FromDouble(value);
}

PyObject* convertPython(lua_State* L, int index) {
    /* Normalise to an absolute index: the table branch pushes values, and a
     * negative index would silently point somewhere else afterwards. */
    if (index < 0) {
        index = lua_gettop(L) + index + 1;
    }
    int type = lua_type(L, index);

    if (type == LUA_TUSERDATA) {
        if (!isPythonObject(L, index)) {
            luaL_error(L, "convertPython: %s is not a Python object", luaL_typename(L, index));
            return NULL;
        }
        PyObject* obj = *(PyObject**)lua_touserdata(L, index);
        if (obj == NULL) {
            luaL_error(L, "convertPython: the Python object is no longer valid");
            return NULL;
        }
        Py_XINCREF(obj);
        return obj;
    }
    if (type == LUA_TSTRING) {
        return convertStringPython(L, index);
    }
    if (type == LUA_TNUMBER) {
        return convertNumberPython(L, index);
    }
    if (type == LUA_TBOOLEAN) {
        PyObject* py_bool = lua_toboolean(L, index) ? Py_True : Py_False;
        Py_XINCREF(py_bool);
        return py_bool;
    }
    if (type == LUA_TNIL) {
        Py_XINCREF(Py_None);
        return Py_None;
    }
    if (type == LUA_TTABLE) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_should_convert_to_dict);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            loadTools(L);
            lua_rawgeti(L, LUA_REGISTRYINDEX, tools_should_convert_to_dict);
        }
        lua_pushvalue(L, index);
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            luaL_error(L, "convertPython: shouldConvertToDict failed: %s",
                       err != NULL ? err : "unknown error");
            return NULL;
        }
        int is_dict = lua_toboolean(L, -1);
        lua_pop(L, 1);
        if (is_dict) {
            return convertDictPython(L, index);
        }
        return convertListPython(L, index);
    }
    luaL_error(L, "convertPython: unsupported Lua type %s", luaL_typename(L, index));
    return NULL;
}

int luaopen_luapython_core(lua_State* L) {
    lua_createtable(L, 0, 10);

    /* import.lua turns the raw importer into the user facing one:
     * `wrap(python_import)` -> function(module_name). */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "luapython.import");
    lua_call(L, 1, 1);                 /* import module table */
    lua_pushcfunction(L, python_import);
    lua_call(L, 1, 1);                 /* adapted import function */
    lua_setfield(L, -2, "import");

    lua_pushcfunction(L, python_set);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, python_dict);
    lua_setfield(L, -2, "dict");
    lua_pushcfunction(L, python_tuple);
    lua_setfield(L, -2, "tuple");
    lua_pushcfunction(L, python_list);
    lua_setfield(L, -2, "list");
    lua_pushcfunction(L, luapython_astable);
    lua_setfield(L, -2, "astable");

    lua_rawgeti(L, LUA_REGISTRYINDEX, tools_release_to_env);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        loadTools(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_release_to_env);
    }
    lua_setfield(L, -2, "init");

    lua_pushcfunction(L, python_initialize);
    lua_setfield(L, -2, "initialize");
    lua_pushcfunction(L, python_finalize);
    lua_setfield(L, -2, "finalize");
    lua_pushcfunction(L, python_addr);
    lua_setfield(L, -2, "addr");
    return 1;
}

int luaopen_luapython_core_import(lua_State* L) {
    lua_pushcfunction(L, python_import);
    return 1;
}
