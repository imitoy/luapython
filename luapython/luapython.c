#include "luapython.h"

static int python_initialize(lua_State* L) {
    if (Py_IsInitialized()) {
        luaL_error(L, "luapython has been loaded");
        return 0;
    }
    Py_Initialize();
    return 0;
}

static int python_finalize(lua_State* L) {
    if (!Py_IsInitialized()) {
        luaL_error(L, "luapython has not been loaded");
        return 0;
    }
    Py_Finalize();
    return 0;
}

static int python_import(lua_State* L) {
    if(!Py_IsInitialized()){
        luaL_error(L, "luapython has not been loaded");
    }
    if (!lua_isstring(L, -2)) {
        luaL_error(L, luaL_typename(L, -2));
        return 0;
    }
    const char* module_name = lua_tostring(L, -2);
    PyObject* module = PyImport_Import(PyUnicode_FromString(module_name));
    if (module == NULL) {
        bool b = PyErr_Occurred();
        if (lua_toboolean(L, -1)) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        return 0;
    }
    pushLua(L, module);
    Py_XDECREF(module);
    return 1;
}

static int python_set(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python set", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* set = convertSetPython(L, -1);
    pushSetLua(L, set);
    Py_XDECREF(set);
    return 1;
}

static int python_dict(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python dict", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* dict = convertDictPython(L, -1);
    pushDictLua(L, dict);
    Py_XDECREF(dict);
    return 1;
}

static int python_tuple(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python tuple", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* tuple = convertTuplePython(L, -1);
    pushTupleLua(L, tuple);
    Py_XDECREF(tuple);
    return 1;
}

static int python_list(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python list", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* list = convertListPython(L, -1);
    pushListLua(L, list);
    Py_XDECREF(list);
    return 1;
}

int python_tostring(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        luaL_error(L, "python_tostring: Not a Python object");
        return 0;
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    if(Py_IsNone(obj)){
        lua_pushnil(L);
        return 1;
    }
    PyObject* str = PyObject_Str(obj);
    if(PyErr_Occurred()){
        PyErr_Print();
    }
    if (!str) {
        luaL_error(L, "python_tostring: Failed to convert Python object to string");
        return 0;
    }
    pushStringLua(L, str);
    Py_XDECREF(str);
    return 1;
}

int python_gc(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        luaL_error(L, "python_gc: Not a Python object");
        return 0;
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    Py_XDECREF(obj);
    return 0;
}

int python_index(lua_State* L) {
    if (!lua_isuserdata(L, -2)) {
        luaL_error(L, "python_index: Attempt to index a %s value", luaL_typename(L, -2));
        return 0;
    }
    const char* key = lua_tostring(L, -1);
    if (!isPythonObject(L, -2)) {
        luaL_error(L, "python_index: Not a Python object");
        return 0;
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -2);
    if (!PyObject_HasAttrString(obj, key)) {
        lua_pushnil(L);
        return 1;
    }
    PyObject* attr = PyObject_GetAttrString(obj, key);
    pushLua(L, attr);
    Py_XDECREF(attr);
    return 1;
}

int python_addr(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        luaL_error(L, "python_addr: Not a Python object");
        return 0;
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    lua_pushfstring(L, "%p", obj);
    return 1;
}

bool isPythonObject(lua_State* L, int index) {
    if (lua_getmetatable(L, index) == 1) {
        lua_getfield(L, -1, "__name");
        if (lua_type(L, -1) == LUA_TSTRING) {
            const char* name = lua_tostring(L, -1);
            lua_pop(L, 2);
            return strncmp(name, PYTHON_OBJECT_NAME, 6) == 0;
        }
        lua_pop(L, 2);
    }
    return false;
}

int pushLua(lua_State* L, PyObject* obj) {
    if(obj == NULL || Py_IsNone(obj)) {
        lua_pushnil(L);
        return 1;
    }else if (PyBool_Check(obj)) {
        lua_pushboolean(L, obj == Py_True);
        return 1;
    }else if (PyNumber_Check(obj)) {
        return pushNumberLua(L, obj);
    } else if (PyUnicode_Check(obj)) {
        return pushStringLua(L, obj);
    } else if (PySet_Check(obj)) {
        return pushSetLua(L, obj);
    } else if (PyDict_Check(obj)) {
        return pushDictLua(L, obj);
    } else if (PyTuple_Check(obj)) {
        return pushTupleLua(L, obj);
    } else if (PyList_Check(obj)) {
        return pushListLua(L, obj);
    } else if (PyModule_Check(obj)) {
        return pushModuleLua(L, obj);
    } else if (PyCallable_Check(obj)) {
        return pushFunctionLua(L, obj);
    } else if (PyIter_Check(obj)) {
        return pushIterLua(L, obj);
    } else {
        return pushClassLua(L, obj);
    }
}

PyObject* convertPython(lua_State* L, int index) {
    if (lua_isuserdata(L, index)) {
        PyObject* obj = *((PyObject**)lua_touserdata(L, index));
        Py_XINCREF(obj);
        return obj;
    } else if (lua_type(L, index) == LUA_TSTRING) {
        return convertStringPython(L, index);
    } else if (lua_type(L, index) == LUA_TNUMBER) {
        return convertNumberPython(L, index);
    } else if (lua_isboolean(L, index)) {
        return lua_toboolean(L, index) ? Py_True : Py_False;
    } else if (lua_isnil(L, index)) {
        Py_XINCREF(Py_None);
        return Py_None;
    } else if (lua_istable(L, index)) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_should_convert_to_dict);
        if(lua_isnil(L, -1)){
            loadTools(L);
            lua_pop(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, tools_should_convert_to_dict);
        }
        lua_pushvalue(L, index > 0 ? index + 1 : index - 1);
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            luaL_error(L, "Error running function `convert`: %s", lua_tostring(L, -1));
            return NULL;
        }
        PyObject* pyobject = NULL;
        bool is_dict = lua_toboolean(L, -1);
        lua_pop(L, 1);
        if (is_dict) {
            pyobject = convertDictPython(L, index);
        } else {
            pyobject = convertListPython(L, index);
        }
        return pyobject;
    }
    luaL_error(L, "Unsupported Lua type for conversion to Python: %s", luaL_typename(L, index));
    return NULL;
}

int luaopen_luapython_core(lua_State* L) {
    lua_createtable(L, 0, 10);
    if(luaL_dostring(L, "local lib = require(\"luapython.import\") return lib") != LUA_OK){
        luaL_error(L, "luaopen_luapython_core: Failed to load internal tools");
    }
    lua_pushcfunction(L, python_import);
    if(lua_pcall(L, 1, 1, 0) != LUA_OK){
        luaL_error(L, "luaopen_luapython_core: Failed to get import function: %s", lua_tostring(L, -1));
    }
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
    if(lua_isnil(L, -1)){
        loadTools(L);
        lua_pop(L, 1);
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_release_to_env);
    }
    lua_setfield(L, -2, "init");
    lua_pushcfunction(L, python_initialize);
    lua_setfield(L, -2, "initialize");
    lua_pushcfunction(L, python_finalize);
    lua_setfield(L, -2, "finalize");
    lua_pushcfunction(L, python_addr);
    lua_setfield(L, -2, "addr");
    int d = tools_release_to_env;
    return 1;
}

int luaopen_luapython_core_import(lua_State* L) {
    lua_pushcfunction(L, python_import);
    return 1;
}
