#include <Python.h>
#include <lua.hpp>
//#include <iostream>
#include <dlfcn.h>
#include "luapython.hpp"

static int python_import(lua_State *L)
{
    if (!lua_isstring(L, -1))
    {
        luaL_error(L, luaL_typename(L, -1));
        return 0;
    }
    const char* module_name = lua_tostring(L, -1);
    PyObject* module = PyImport_Import(PyUnicode_FromString(module_name));
    if (module == nullptr)
    {
        luaL_error(L, "import error: no such python module: ", module_name);
        return 0;
    }
    pushLua(L, module);
    return 1;
}

static int python_set(lua_State* L){
    if(!lua_istable(L, -1)){
        luaL_error(L, "Attempt to convert %s to python set", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* set = convertSetPython(L, -1);
    pushSetLua(L, set);
    return 1;
}

static int python_dict(lua_State* L){
    if(!lua_istable(L, -1)){
        luaL_error(L, "Attempt to convert %s to python dict", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* dict = convertDictPython(L, -1);
    pushDictLua(L, dict);
    return 1;
}

static int python_tuple(lua_State* L){
    if(!lua_istable(L, -1)){
        luaL_error(L, "Attempt to convert %s to python tuple", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* tuple = convertTuplePython(L, -1);
    pushTupleLua(L, tuple);
    return 1;
}

static int python_list(lua_State* L){
    if(!lua_istable(L, -1)){
        luaL_error(L, "Attempt to convert %s to python list", lua_typename(L, lua_type(L, -1)));
    }
    PyObject* list = convertListPython(L, -1);
    pushListLua(L, list);
    return 1;
}

static int table_index(lua_State *L)
{
    if (!lua_isstring(L, -1))
    {
        luaL_error(L, "valid index");
        return 0;
    }
    const char* key = lua_tostring(L, -1);
    PyObject* module = *(PyObject**)lua_touserdata(L, -2);
    PyObject* value = PyObject_GetAttrString(module, key);
    const char* s = Py_TYPE(module)->tp_name;
    const char *t = Py_TYPE(value)->tp_name;
    return 1;
}

int python_gc(lua_State *L)
{
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    //std::cout << "Python object deleted" << std::endl;
    Py_DECREF(obj);
    return 0;
}

int pushLua(lua_State*L, PyObject* obj) {
    if(PyNumber_Check(obj)) {
        return pushNumberLua(L, obj);
    } else if(PyUnicode_Check(obj)) {
        return pushStringLua(L, obj);
    } else if(PySet_Check(obj)) {
        return pushSetLua(L, obj);
    } else if(PyDict_Check(obj)) {
        return pushDictLua(L, obj);
    } else if(PyModule_Check(obj)) {
        return pushModuleLua(L, obj);
    } else if(PyCallable_Check(obj)) {
        return pushFunctionLua(L, obj);
    } else {
        luaL_error(L, "Unsupported Python object type: %s for Lua metatable", Py_TYPE(obj)->tp_name);
        return 0;
    }
}

PyObject* convertPython(lua_State*L, int index) {
    if(lua_isuserdata(L, index)) {
        PyObject* obj = *((PyObject**)lua_touserdata(L, index));
        Py_INCREF(obj);
        return obj;
    } else if(lua_type(L, index) == LUA_TSTRING) {
        return convertStringPython(L, index);
    } else if(lua_type(L, index) == LUA_TNUMBER) {
        return convertNumberPython(L, index);
    } else if(lua_isboolean(L, index)) {
        return lua_toboolean(L, index) ? Py_True : Py_False;
    } else if(lua_isnil(L, index)) {
        Py_INCREF(Py_None);
        return Py_None;
    } else if(lua_istable(L, index)){
        int ret = luaL_loadfile(L, "convert_pre.lua");
        if(ret != LUA_OK) {
            luaL_error(L, "Failed to load convert_pre.lua: %s", lua_tostring(L, -1));
            return nullptr;
        }
        lua_pushvalue(L, index);
        if(lua_pcall(L, 1, 1, 0) != LUA_OK) {
            luaL_error(L, "Error running function `convert`: %s", lua_tostring(L, -1));
            return nullptr;
        }
        PyObject* pyobject = nullptr;
        if(lua_toboolean(L, -1)) {
            pyobject = convertDictPython(L, index);
        }else{
            pyobject = convertListPython(L, index);
        }
        lua_pop(L, 1);
        return pyobject;
    }
    luaL_error(L, "Unsupported Lua type for conversion to Python: %s", luaL_typename(L, index));
    return nullptr; // 不会到达这里
}

extern "C" int luaopen_luapython(lua_State *L)
{
    void* handle = dlopen("libpython3.13.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        // 如果加载失败，报告错误
        luaL_error(L, "Failed to load libpython3.13.so: %s", dlerror());
        return 0;
    }
    if(!Py_IsInitialized()) {
        Py_Initialize();
    }
    lua_createtable(L, 0, 6);
    lua_pushcfunction(L, python_import);
    lua_setfield(L, -2, "import");
    lua_pushcfunction(L, python_set);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, python_dict);
    lua_setfield(L, -2, "dict");
    lua_pushcfunction(L, python_tuple);
    lua_setfield(L, -2, "tuple");
    lua_pushcfunction(L, python_list);
    lua_setfield(L, -2, "list");
    luaL_loadfile(L, "python_init.lua");
    lua_setfield(L, -2, "init");
    return 1;
}

extern "C" int luaopen_luapython_import(lua_State *L){
    void* handle = dlopen("libpython3.13.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        // 如果加载失败，报告错误
        luaL_error(L, "Failed to load libpython3.13.so: %s", dlerror());
        return 0;
    }
    if(!Py_IsInitialized()) {
        Py_Initialize();
    }
    lua_pushcfunction(L, python_import);
    return 1;
}

extern "C" int luaopen_luapython_dict(lua_State *L){
    void* handle = dlopen("libpython3.13.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        // 如果加载失败，报告错误
        luaL_error(L, "Failed to load libpython3.13.so: %s", dlerror());
        return 0;
    }
    if(!Py_IsInitialized()) {
        Py_Initialize();
    }
    lua_pushcfunction(L, python_dict);
    return 1;
}

int main()
{
    //std::cout << "Hello World!" << std::endl;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaopen_luapython(L);
    lua_setglobal(L, "luapython");
    luaL_loadfile(L, "example.lua");
    //std::cout << "Run lua file" << std::endl;
    lua_call(L, 0, 1);
    luaopen_luapython_dict(L);
    lua_createtable(L, 0, 2);
    lua_pushstring(L, "Name");
    lua_setfield(L, -2, "name");
    lua_pushinteger(L, 18);
    lua_setfield(L, -2, "age");
    if(lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        //std::cout << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    //查看lua栈的状态
    for(int i = 1; i <= lua_gettop(L); i++){
        //std::cout << "Stack " << i << ": " << lua_typename(L, lua_type(L, i)) << std::endl;
    }


    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        //std::cout << lua_tostring(L, -1) << std::endl;
    }
    

    lua_close(L);
    return 0;
}