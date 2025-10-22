#include "luapython.h"

int luapython_astable(lua_State* L) {
    if(!isPythonObject(L, -1)) {
        luaL_error(L, "luapython_astable: Not a Python object");
        return 0;
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    PyObject* iter = PyObject_GetIter(obj);
    if(PyErr_Occurred()) {
        PyErr_Clear();
    }
    if(iter) {
        lua_newtable(L);
        PyObject* item;
        int index = 1;
        while((item = PyIter_Next(iter)) != NULL) {
            pushLua(L, item);
            lua_rawseti(L, -2, index++);
            Py_XDECREF(item);
        }
        return 1;
    }
    PyObject* dir = PyObject_Dir(obj);
    if(PyErr_Occurred()) {
        PyErr_Print();
        luaL_error(L, "luapython_astable: Failed to get attributes of Python object");
        return 0;
    }
    if(!PyList_Check(dir)) {
        Py_XDECREF(dir);
        luaL_error(L, "luapython_astable: Internal error, PyObject_Dir did not return a list");
        return 0;
    }
    Py_ssize_t size = PyList_Size(dir);
    lua_createtable(L, 0, (int)size);
    for(Py_ssize_t index = 0; index < size; index++) {
        PyObject* item = PyList_GetItem(dir, index);
        if(!PyUnicode_Check(item)) {
            continue;
        }
        const char* key = PyUnicode_AsUTF8(item);
        if(key == NULL) {
            PyErr_Print();
            luaL_error(L, "luapython_astable: Failed to convert attribute name to UTF-8");
            Py_XDECREF(dir);
            return 0;
        }
        PyObject* value = PyObject_GetAttrString(obj, key);
        if(value == NULL) {
            PyErr_Clear();
            continue;
        }
        int ret = pushLua(L, value);
        Py_XDECREF(value);
        if(ret != 1) {
            luaL_error(L, "luapython_astable: Failed to push attribute value to Lua stack");
            Py_XDECREF(dir);
            return 0;
        }
        lua_setfield(L, -2, key);
    }
    Py_XDECREF(dir);
    return 1;
}