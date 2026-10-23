#include "luapython.h"

int class_index(lua_State* L) {
    if (!isPythonObject(L, -2)) {
        return luaL_error(L, "class_index: attempt to index a %s value", luaL_typename(L, -2));
    }
    int key_type = lua_type(L, -1);
    if (key_type != LUA_TSTRING && key_type != LUA_TNUMBER) {
        return luaL_error(L, "class_index: attribute name must be a string, got %s",
                          luaL_typename(L, -1));
    }
    const char* key = lua_tostring(L, -1);
    PyObject* obj = *(PyObject**)lua_touserdata(L, -2);
    if (obj == NULL) {
        return luaL_error(L, "class_index: the Python object is no longer valid");
    }
    PyObject* attr = PyObject_GetAttrString(obj, key);
    if (attr == NULL) {
        /* Same contract as Python's getattr(obj, name, None): a missing
         * attribute yields nil, but an exception raised inside a property is
         * a real error and must not be swallowed. */
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

int pushClassLua(lua_State* L, PyObject* obj) {
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    /* luaL_newmetatable() caches the metatable in *this* state's registry, so
     * no cross-state global bookkeeping is needed. */
    if (luaL_newmetatable(L, PYTHON_CLASS_NAME)) {
        lua_pushcfunction(L, class_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, python_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, python_tostring);
        lua_setfield(L, -2, "__tostring");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}
