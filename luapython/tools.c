/* luapython.h first: it pulls in Python.h, which must precede the standard
 * headers (see the CPython "include Python.h first" rule). */
#include "luapython.h"

/* Registry references to the Lua helpers in luapython/tools.lua, filled in
 * lazily by loadTools(). -1 means "not loaded yet". */
int tools_should_convert_to_dict = -1;
int tools_release_to_env = -1;
int tools_get_python_adapt_function = -1;
int tools_get_iter_function = -1;

/* Convert a Python object into a Lua table.
 * Iterable  -> array of the yielded items (consumes the iterator).
 * Otherwise -> table of attribute name -> value (like dir()). */
int luapython_astable(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        return luaL_error(L, "luapython.astable: not a Python object");
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    if (obj == NULL) {
        return luaL_error(L, "luapython.astable: the Python object is no longer valid");
    }

    PyObject* iter = PyObject_GetIter(obj);
    if (iter != NULL) {
        lua_newtable(L);
        PyObject* item;
        int index = 1;
        while ((item = PyIter_Next(iter)) != NULL) {
            pushLua(L, item);        /* consumes */
            lua_rawseti(L, -2, index++);
        }
        Py_DECREF(iter);
        if (PyErr_Occurred()) {
            /* Stop silently truncating the table on an error inside the
             * iterator. */
            return luapython_throw(L, "luapython.astable");
        }
        return 1;
    }
    if (!PyErr_ExceptionMatches(PyExc_TypeError)) {
        return luapython_throw(L, "luapython.astable");
    }
    PyErr_Clear();                   /* not iterable: fall back to attributes */

    PyObject* dir = PyObject_Dir(obj);
    if (dir == NULL) {
        return luapython_throw(L, "luapython.astable");
    }
    if (!PyList_Check(dir)) {
        Py_DECREF(dir);
        return luaL_error(L, "luapython.astable: internal error, dir() did not return a list");
    }
    Py_ssize_t size = PyList_Size(dir);
    lua_createtable(L, 0, (int)size);
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject* name = PyList_GetItem(dir, i);   /* borrowed */
        if (!PyUnicode_Check(name)) {
            continue;
        }
        PyObject* bytes = PyUnicode_AsEncodedString(name, "utf-8", "surrogateescape");
        if (bytes == NULL) {
            PyErr_Clear();
            continue;
        }
        const char* key = PyBytes_AsString(bytes);
        if (key == NULL) {
            Py_DECREF(bytes);
            continue;
        }
        PyObject* value = PyObject_GetAttr(obj, name);
        if (value == NULL) {
            PyErr_Clear();                         /* properties may raise */
            Py_DECREF(bytes);
            continue;
        }
        pushLua(L, value);                         /* consumes */
        lua_setfield(L, -2, key);
        Py_DECREF(bytes);
    }
    Py_DECREF(dir);
    return 1;
}

/* --- internal Lua helpers ------------------------------------------------- */

static int ref_field(lua_State* L, int module, const char* name) {
    lua_getfield(L, module, name);
    if (!lua_isfunction(L, -1)) {
        return luaL_error(L, "loadTools: luapython.tools.%s is not a function", name);
    }
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

void loadTools(lua_State* L) {
    int base = lua_gettop(L);

    lua_getglobal(L, "require");
    lua_pushliteral(L, "luapython.tools");
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        luaL_error(L, "loadTools: cannot load luapython.tools: %s",
                   lua_tostring(L, -1) != NULL ? lua_tostring(L, -1) : "unknown error");
    }
    int module = lua_gettop(L);   /* absolute: immune to stack shifts below */

    tools_should_convert_to_dict = ref_field(L, module, "shouldConvertToDict");
    tools_release_to_env = ref_field(L, module, "releaseToEnv");
    tools_get_python_adapt_function = ref_field(L, module, "getPythonAdaptFunction");
    tools_get_iter_function = ref_field(L, module, "getIterFunction");

    lua_settop(L, base);
}
