#include "luapython.h"

#define isPythonIter(L, index) (isPythonObject(L, index) && PyIter_Check(*(PyObject**)lua_touserdata(L, index)))

/* Step function of the generic-for produced by pushIterLua()/tools.lua.
 * Receives (state, control) and returns the next item, or nil at the end. */
int lua_iter(lua_State* L) {
    if (!isPythonIter(L, -2)) {
        return luaL_error(L, "lua_iter: not a Python iterator");
    }
    PyObject* iter = *(PyObject**)lua_touserdata(L, -2);
    if (iter == NULL) {
        return luaL_error(L, "lua_iter: the Python iterator is no longer valid");
    }
    PyObject* next = PyIter_Next(iter);
    if (next == NULL) {
        if (PyErr_Occurred()) {
            /* An exception inside the generator must not look like the end of
             * the sequence. */
            return luapython_throw(L, "lua_iter");
        }
        lua_pushnil(L);
        return 1;
    }
    pushLua(L, next);   /* consumes */
    return 1;
}

int lua_getiter(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        return luaL_error(L, "lua_getiter: not a Python object");
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    if (obj == NULL) {
        return luaL_error(L, "lua_getiter: the Python object is no longer valid");
    }
    PyObject* iter = PyObject_GetIter(obj);
    if (iter == NULL) {
        return luapython_throw(L, "lua_getiter");
    }
    pushClassLua(L, iter);   /* consumes; iterator objects use the class metatable */
    return 1;
}

int pushIterLua(lua_State* L, PyObject* iter) {
    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = iter;
    if (luaL_newmetatable(L, PYTHON_ITER_NAME)) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_get_iter_function);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            loadTools(L);
            lua_rawgeti(L, LUA_REGISTRYINDEX, tools_get_iter_function);
        }
        lua_pushcfunction(L, lua_iter);
        lua_pushcfunction(L, lua_getiter);
        lua_call(L, 2, 1);                 /* getIterFunction(lua_iter, lua_getiter) */
        lua_setfield(L, -2, "__call");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, python_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, python_index);
        lua_setfield(L, -2, "__index");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}
