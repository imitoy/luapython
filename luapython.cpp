#include "luapython.hpp"
#include <dlfcn.h>
#include <string>

#ifndef PREFIX
#define PREFIX "/usr"
#endif

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
    if (module == nullptr) {
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
        std::string prefix = PREFIX;
        std::string name = "/local/lib/lua/5.4/luapython/convert_pre.lua";
        std::string path = prefix + name;
        int ret = luaL_loadfile(L, path.c_str());
        if (ret != LUA_OK) {
            luaL_error(L, "Failed to load convert_pre.lua: %s", lua_tostring(L, -1));
            return nullptr;
        }
        lua_pushvalue(L, index > 0 ? index + 1 : index - 1);
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            luaL_error(L, "Error running function `convert`: %s", lua_tostring(L, -1));
            return nullptr;
        }
        PyObject* pyobject = nullptr;
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
    return nullptr;
}

extern "C" int luaopen_luapython(lua_State* L) {
    void* handle = dlopen(PYTHON_LIB, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        luaL_error(L, "Failed to load %s: %s", PYTHON_LIB, dlerror());
        return 0;
    }
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
    lua_createtable(L, 0, 6);
    std::string prefix1 = PREFIX;
    std::string name1 = "/local/lib/lua/5.4/luapython/import.lua";
    std::string path1 = prefix1 + name1;
    luaL_loadfile(L, path1.c_str());
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
    std::string prefix2 = PREFIX;
    std::string name2 = "/local/lib/lua/5.4/luapython/python_init.lua";
    std::string path2 = prefix2 + name2;
    luaL_loadfile(L, path2.c_str());
    lua_setfield(L, -2, "init");
    return 1;
}

extern "C" int luaopen_luapython_import(lua_State* L) {
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
    luaL_loadfile(L, "/home/imitoy/Coding/MiniData/get.lua");
    lua_pcall(L, 0, 0, 0);
    const char* s = lua_tostring(L, -1);
    lua_close(L);
    return 0;
}