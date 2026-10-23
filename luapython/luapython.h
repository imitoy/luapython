/* Python.h must come first: it re-defines feature-test macros that affect the
 * standard headers (see the CPython "Include Python.h first" rule). */
#define Py_LIMITED_API 0x03080000

#include <Python.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "tools.h"

#ifndef PREFIX
#define PREFIX "/usr"
#endif

#define PYTHON_OBJECT_NAME "python_object"
#define PYTHON_CLASS_NAME "python_class"
#define PYTHON_MODULE_NAME "python_module"
#define PYTHON_FUNCTION_NAME "python_function"
#define PYTHON_ITER_NAME "python_iter"
#define PYTHON_SET_NAME "python_set"
#define PYTHON_DICT_NAME "python_dict"
#define PYTHON_TUPLE_NAME "python_tuple"
#define PYTHON_LIST_NAME "python_list"
#define PYTHON_STRING_NAME "python_string"
#define PYTHON_NUMBER_NAME "python_number"

/* Registry key of the per-state set holding every metatable created by this
 * library. Object checks compare metatable *identity* through that set, so a
 * script cannot fake a Python object with debug.setmetatable(). */
#define LUAPYTHON_METATABLES "luapython.metatables"

#define isPythonTuple(L, index) (isPythonObject(L, index) && PyTuple_Check(*(PyObject**)lua_touserdata(L, index)))

#if LUA_VERSION_NUM <= 501
#define LUA_OK 0
#define lua_rawlen lua_objlen
#endif

int luaopen_luapython(lua_State* L);

int python_tostring(lua_State* L);
int python_gc(lua_State* L);
int python_index(lua_State* L);
int python_newindex(lua_State* L);

int isPythonObject(lua_State* L, int index);

/* --- error reporting ----------------------------------------------------- */

/* Raise a Lua error carrying the pending Python exception.
 * Always clears the Python error state, so a failure can never poison the
 * following, unrelated Python calls. Never returns. */
int luapython_throw(lua_State* L, const char* context);

/* Write the type name (`__name__`) of `obj` into `buf`. Never raises. */
void luapython_type_name(PyObject* obj, char* buf, size_t size);

/* --- metatable bookkeeping ------------------------------------------------ */

/* Record the metatable on top of the stack as a "known luapython metatable".
 * Leaves the stack unchanged. Must be called exactly once per metatable. */
void luapython_register_metatable(lua_State* L);

/* --- numbers -------------------------------------------------------------- */

/* Convert the Lua number at `index` to a new Python reference.
 *
 * Lua 5.3+ has an integer subtype, so `1` becomes int and `1.0` stays float.
 * Lua 5.1/5.2 does not: every number is a double and `1 == 1.0`. We map
 * integral doubles to PyLong so that library calls which require an int keep
 * working there (notably Fraction(1, 2), which raises "both arguments should
 * be Rational instances" when handed floats). Returns NULL if the value is
 * not a Lua number. */
PyObject* luapython_number_to_py(lua_State* L, int index);

/* --- pushes / conversions ------------------------------------------------- */

/* All push* functions CONSUME one reference to `obj` (they take ownership).
 * Callers holding a borrowed reference must Py_XINCREF() beforehand. */
int pushNumberLua(lua_State* L, PyObject* number);
int pushStringLua(lua_State* L, PyObject* string);
int pushSetLua(lua_State* L, PyObject* set);
int pushDictLua(lua_State* L, PyObject* dict);
int pushTupleLua(lua_State* L, PyObject* tuple);
int pushListLua(lua_State* L, PyObject* list);
int pushFunctionLua(lua_State* L, PyObject* function);
int pushModuleLua(lua_State* L, PyObject* module);
int pushClassLua(lua_State* L, PyObject* obj);
int pushIterLua(lua_State* L, PyObject* iter);

int pushLua(lua_State* L, PyObject* obj);

/* All convert* functions return a NEW reference (NULL on failure). */
PyObject* convertNumberPython(lua_State* L, int index);
PyObject* convertBooleanPython(lua_State* L, int index);
PyObject* convertStringPython(lua_State* L, int index);
PyObject* convertSetPython(lua_State* L, int index);
PyObject* convertDictPython(lua_State* L, int index);
PyObject* convertTuplePython(lua_State* L, int index);
PyObject* convertListPython(lua_State* L, int index);
PyObject* convertFunctionPython(lua_State* L, int index);
PyObject* convertModulePython(lua_State* L, int index);

PyObject* convertPython(lua_State* L, int index);
