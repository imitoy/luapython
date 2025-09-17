#include "luapython.h"
#include <dlfcn.h>

#ifndef PYTHON_LIB
#define PYTHON_LIB "libpython3.13.so"
#endif

static int python_import(lua_State* L) {
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
    return 1;
}

static int python_set(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python set", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* set = convertSetPython(L, -1);
    pushSetLua(L, set);
    return 1;
}

static int python_dict(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python dict", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* dict = convertDictPython(L, -1);
    pushDictLua(L, dict);
    return 1;
}

static int python_tuple(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python tuple", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* tuple = convertTuplePython(L, -1);
    pushTupleLua(L, tuple);
    return 1;
}

static int python_list(lua_State* L) {
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Attempt to convert %s to python list", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* list = convertListPython(L, -1);
    pushListLua(L, list);
    return 1;
}

static int table_index(lua_State* L) {
    if (!lua_isstring(L, -1)) {
        luaL_error(L, "valid index");
        return 0;
    }
    const char* key = lua_tostring(L, -1);
    PyObject* module = *(PyObject**)lua_touserdata(L, -2);
    PyObject* value = PyObject_GetAttrString(module, key);
    const char* s = Py_TYPE(module)->tp_name;
    const char* t = Py_TYPE(value)->tp_name;
    return 1;
}

int python_gc(lua_State* L) {
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    // std::cout << "Python object deleted" << std::endl;
    Py_DECREF(obj);
    return 0;
}

bool isPythonObject(lua_State* L, int index) {
    if (lua_getmetatable(L, index) == 1) {
        lua_getfield(L, -1, "__name");
        if (lua_type(L, -1) == LUA_TSTRING) {
            const char* name = lua_tostring(L, -1);
            lua_pop(L, 2);
            return strcmp(name, PYTHON_OBJECT_NAME) == 0;
        }
        lua_pop(L, 2);
    }
    return false;
}

int pushLua(lua_State* L, PyObject* obj) {
    if (PyNumber_Check(obj)) {
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
    } else {
        const char* type_name = Py_TYPE(obj)->tp_name;
        return pushClassLua(L, obj);
    }
}

PyObject* convertPython(lua_State* L, int index) {
    if (lua_isuserdata(L, index)) {
        PyObject* obj = *((PyObject**)lua_touserdata(L, index));
        Py_INCREF(obj);
        return obj;
    } else if (lua_type(L, index) == LUA_TSTRING) {
        return convertStringPython(L, index);
    } else if (lua_type(L, index) == LUA_TNUMBER) {
        return convertNumberPython(L, index);
    } else if (lua_isboolean(L, index)) {
        return lua_toboolean(L, index) ? Py_True : Py_False;
    } else if (lua_isnil(L, index)) {
        Py_INCREF(Py_None);
        return Py_None;
    } else if (lua_istable(L, index)) {
        const char prefix[] = PREFIX;
        const char name[] = "/local/lib/lua/5.4/luapython/convert_pre.lua";
        char path[strlen(prefix) + strlen(name) + 1];
        strcpy((char*)path, prefix);
        strcat((char*)path, name);
        int ret = luaL_loadfile(L, path);
        if (ret != LUA_OK) {
            luaL_error(L, "Failed to load convert_pre.lua: %s", lua_tostring(L, -1));
            return NULL;
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

int luaopen_luapython(lua_State* L) {
    void* handle = dlopen(PYTHON_LIB, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        luaL_error(L, "Failed to load %s: %s", PYTHON_LIB, dlerror());
        return 0;
    }
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
    lua_createtable(L, 0, 6);
    const char prefix[] = PREFIX;
    const char name[] = "/local/lib/lua/5.4/luapython/import.lua";
    char path1[strlen(prefix) + strlen(name) + 1];
    strcpy((char*)path1, prefix);
    strcat((char*)path1, name);
    luaL_loadfile(L, path1);
    lua_pushcfunction(L, python_import);
    lua_call(L, 1, 1);
    lua_setfield(L, -2, "import");
    lua_pushcfunction(L, python_set);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, python_dict);
    lua_setfield(L, -2, "dict");
    lua_pushcfunction(L, python_tuple);
    lua_setfield(L, -2, "tuple");
    lua_pushcfunction(L, python_list);
    lua_setfield(L, -2, "list");
    const char prefix2[] = PREFIX;
    const char name2[] = "/local/lib/lua/5.4/luapython/python_init.lua";
    char path2[strlen(prefix2) + strlen(name2) + 1];
    strcpy((char*)path2, prefix2);
    strcat((char*)path2, name2);
    luaL_loadfile(L, path2);
    lua_setfield(L, -2, "init");
    //print python version
    printf("Python version: %s\n", Py_GetVersion());
    return 1;
}

int luaopen_luapython_import(lua_State* L) {
    void* handle = dlopen(PYTHON_LIB, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        luaL_error(L, "Failed to load %s: %s", PYTHON_LIB, dlerror());
        return 0;
    }
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
    lua_pushcfunction(L, python_import);
    return 1;
}

int main() {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    luaL_loadfile(L, "/mnt/e.lua");
    lua_pcall(L, 0, 0, 0);
    const char* s = lua_tostring(L, -1);
    lua_close(L);
    return 0;
}