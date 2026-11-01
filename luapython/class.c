#include "luapython.h"

int class_index(lua_State* L) {
    if (!isPythonObject(L, -2)) {
        return luaL_error(L, "class_index: attempt to index a %s value", luaL_typename(L, -2));
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -2);
    if (obj == NULL) {
        return luaL_error(L, "class_index: the Python object is no longer valid");
    }
    /* getattr() first, __getitem__ second: see luapython_index_shared(). */
    return luapython_index_shared(L, obj, -1);
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
        lua_pushcfunction(L, python_object_eq);
        lua_setfield(L, -2, "__eq");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}
