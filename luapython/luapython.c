#include "luapython.h"

#include <stdlib.h>

/* Set once Py_Finalize() has run. Every handle handed to Lua before that point
 * is a dangling pointer; this flag keeps isPythonObject() from trusting them
 * even if somebody manages to start an interpreter again. */
static int g_finalized = 0;

/* ------------------------------------------------------------------------- */
/* diagnostics                                                               */
/* ------------------------------------------------------------------------- */

static void msg_appendf(char* buf, size_t size, size_t* used, const char* fmt, ...) {
    va_list ap;
    if (*used >= size) {
        return;
    }
    va_start(ap, fmt);
    int n = vsnprintf(buf + *used, size - *used, fmt, ap);
    va_end(ap);
    if (n > 0) {
        size_t add = (size_t)n;
        if (add >= size - *used) {
            add = size - *used - 1;
        }
        *used += add;
    }
}

static void msg_append_unicode(char* buf, size_t size, size_t* used, PyObject* str) {
    if (str == NULL) {
        return;
    }
    PyObject* bytes = PyUnicode_AsEncodedString(str, "utf-8", "surrogateescape");
    if (bytes == NULL) {
        PyErr_Clear();
        return;
    }
    const char* text = PyBytes_AsString(bytes);
    if (text != NULL) {
        msg_appendf(buf, size, used, "%s", text);
    }
    Py_DECREF(bytes);
}

void luapython_type_name(PyObject* obj, char* buf, size_t size) {
    if (buf == NULL || size == 0) {
        return;
    }
    buf[0] = '\0';
    if (obj == NULL) {
        return;
    }
    PyObject* name = PyObject_GetAttrString((PyObject*)Py_TYPE(obj), "__name__");
    if (name == NULL) {
        PyErr_Clear();
        return;
    }
    size_t used = 0;
    msg_append_unicode(buf, size, &used, name);
    Py_DECREF(name);
}

int luapython_throw(lua_State* L, const char* context) {
    char buf[1024];
    size_t used = 0;

    buf[0] = '\0';
    if (context != NULL) {
        msg_appendf(buf, sizeof buf, &used, "%s: ", context);
    }

    if (!PyErr_Occurred()) {
        msg_appendf(buf, sizeof buf, &used, "Python operation failed");
    } else {
        PyObject *type = NULL, *value = NULL, *tb = NULL;
        PyErr_Fetch(&type, &value, &tb);
        PyErr_NormalizeException(&type, &value, &tb);

        if (type != NULL) {
            PyObject* name = PyObject_GetAttrString(type, "__name__");
            if (name != NULL) {
                msg_append_unicode(buf, sizeof buf, &used, name);
                Py_DECREF(name);
            } else {
                PyErr_Clear();
            }
        }
        if (value != NULL) {
            PyObject* text = PyObject_Str(value);
            if (text != NULL) {
                msg_appendf(buf, sizeof buf, &used, ": ");
                msg_append_unicode(buf, sizeof buf, &used, text);
                Py_DECREF(text);
            } else {
                PyErr_Clear();
            }
        }
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(tb);
        /* Whatever happened above, the interpreter must be left clean: a
         * pending exception would silently degrade the next conversion. */
        PyErr_Clear();
    }

    return luaL_error(L, "%s", buf);
}

/* ------------------------------------------------------------------------- */
/* deferred conversion errors                                                */
/* ------------------------------------------------------------------------- */

/* The recursive table converter cannot use luaL_error(): the longjmp would
 * skip every Py_DECREF between the failure point and the Lua entry point,
 * leaking the containers built so far (and their NULL-initialised PyList
 * slots, which later make repr() dereference NULL). The message is parked in
 * the registry instead, the stack unwinds by returning NULL, and only the Lua
 * entry point raises once all references have been released.
 *
 * The registry is per lua_State, so two independent interpreters cannot see
 * each other's errors. */
#define LUAPYTHON_CONVERT_ERROR "luapython.convert_error"

void luapython_convert_error(lua_State* L, const char* fmt, ...) {
    char buf[768];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    /* Keep the deepest message: outer frames only see a NULL result and must
     * not overwrite the cause with a generic one. */
    lua_getfield(L, LUA_REGISTRYINDEX, LUAPYTHON_CONVERT_ERROR);
    int pending = lua_type(L, -1) == LUA_TSTRING;
    lua_pop(L, 1);
    if (!pending) {
        lua_pushstring(L, buf);
        lua_setfield(L, LUA_REGISTRYINDEX, LUAPYTHON_CONVERT_ERROR);
    }
}

int luapython_convert_error_pending(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAPYTHON_CONVERT_ERROR);
    int pending = lua_type(L, -1) == LUA_TSTRING;
    lua_pop(L, 1);
    return pending;
}

void luapython_convert_error_clear(lua_State* L) {
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAPYTHON_CONVERT_ERROR);
}

int luapython_throw_convert(lua_State* L, const char* context) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAPYTHON_CONVERT_ERROR);
    if (lua_type(L, -1) == LUA_TSTRING) {
        /* Copy before the pop: luaL_error() below may allocate and collect. */
        const char* msg = lua_tostring(L, -1);
        char buf[1024];
        snprintf(buf, sizeof buf, "%s", msg != NULL ? msg : "");
        lua_pop(L, 1);
        luapython_convert_error_clear(L);
        /* A Lua-level conversion failure never leaves a Python exception
         * behind, but clear defensively so a poisoned interpreter state
         * cannot leak into the next, unrelated call. */
        PyErr_Clear();
        return luaL_error(L, "%s", buf);
    }
    lua_pop(L, 1);
    if (PyErr_Occurred()) {
        return luapython_throw(L, context != NULL ? context : "convertPython");
    }
    return luaL_error(L, "%s: conversion failed",
                      context != NULL ? context : "convertPython");
}

/* ------------------------------------------------------------------------- */
/* metatable bookkeeping                                                     */
/* ------------------------------------------------------------------------- */

/* Push the per-state registry table `name`, creating it on first use. */
static void push_state_table(lua_State* L, const char* name) {
    lua_getfield(L, LUA_REGISTRYINDEX, name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_createtable(L, 0, 16);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, name);
    }
}

void luapython_register_metatable(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return;
    }
    lua_pushvalue(L, -1);                          /* mt, mt */
    push_state_table(L, LUAPYTHON_METATABLES);     /* mt, mt, set */
    lua_pushvalue(L, -2);                          /* mt, mt, set, mt */
    lua_pushboolean(L, 1);
    lua_rawset(L, -3);                             /* mt, mt, set */
    lua_pop(L, 2);                                 /* mt */
}

/* Object check by metatable identity: unlike comparing __name strings this
 * cannot be forged from Lua (debug.setmetatable) because the registry set is
 * only reachable through the raw registry index. */
int isPythonObject(lua_State* L, int index) {
    int result = 0;

    if (lua_type(L, index) != LUA_TUSERDATA) {
        return 0;
    }
    if (!Py_IsInitialized() || g_finalized) {
        /* The interpreter is gone: every handle we own points into freed
         * memory, so refuse to touch it instead of crashing. */
        return 0;
    }
    if (lua_getmetatable(L, index) == 0) {
        return 0;
    }
    push_state_table(L, LUAPYTHON_METATABLES);     /* mt, set */
    lua_pushvalue(L, -2);                          /* mt, set, mt */
    lua_rawget(L, -2);                             /* mt, set, value */
    result = lua_toboolean(L, -1);
    lua_pop(L, 3);                                 /* - */
    return result;
}

/* ------------------------------------------------------------------------- */
/* module level functions                                                    */
/* ------------------------------------------------------------------------- */

/* Python's sys.stdout/sys.stderr are block-buffered as soon as they are not a
 * terminal, and CPython only flushes them in Py_FinalizeEx(). A Lua script
 * that never calls luapython.finalize() therefore used to lose every line
 * print()ed by Python code when its output was redirected to a file or a pipe.
 * Finalizing at process exit flushes them (and runs Python's own atexit
 * handlers) exactly once. */
static void luapython_flush_at_exit(void) {
    if (Py_IsInitialized()) {
        /* Same ordering as python_finalize(): every handle we handed to Lua
         * becomes dangling as soon as Py_Finalize() starts, and a __gc that
         * runs during finalization must not try to Py_DECREF one of them. */
        g_finalized = 1;
        Py_FinalizeEx();
    }
}

static int python_initialize(lua_State* L) {
    if (Py_IsInitialized()) {
        return luaL_error(L,
            "python_initialize: the Python interpreter is already running "
            "(started by the host program or by an earlier luapython.load())");
    }
    if (g_finalized) {
        /* Handles obtained before finalize() are dangling; restarting would
         * make isPythonObject() trust them again. */
        return luaL_error(L,
            "python_initialize: the interpreter was finalized earlier in this "
            "process and cannot be restarted");
    }
    Py_Initialize();
    if (!Py_IsInitialized()) {
        return luaL_error(L, "python_initialize: Py_Initialize() failed");
    }
    static int atexit_registered = 0;
    if (!atexit_registered) {
        atexit(luapython_flush_at_exit);
        atexit_registered = 1;
    }
    return 0;
}

static int python_is_finalized(lua_State* L) {
    lua_pushboolean(L, g_finalized);
    return 1;
}

static int python_finalize(lua_State* L) {
    if (!Py_IsInitialized()) {
        return luaL_error(L, "python_finalize: the Python interpreter is not running");
    }
    /* Every handle handed to Lua becomes dangling here. isPythonObject()
     * reports 0 and python_gc() becomes a no-op once Py_Finalize() has run,
     * so surviving objects degrade into inert userdata instead of
     * dereferencing freed memory. */
    g_finalized = 1;
    Py_Finalize();
    return 0;
}

/* Called (through luapython/import.lua) as (module_name, verbose).
 * Returns nil - not an error - when the module cannot be imported, because
 * import.lua relies on that to retry with its dotted-name fallback. */
static int python_import(lua_State* L) {
    if (!Py_IsInitialized()) {
        return luaL_error(L, "luapython has not been loaded; call luapython.load() first");
    }
    const char* module_name = lua_tostring(L, 1);
    if (module_name == NULL) {
        return luaL_error(L, "luapython.import: string expected, got %s", luaL_typename(L, 1));
    }
    int verbose = lua_toboolean(L, 2);

    PyObject* name = PyUnicode_FromString(module_name);
    if (name == NULL) {
        return luapython_throw(L, "luapython.import");
    }
    PyObject* module = PyImport_Import(name);
    Py_DECREF(name);
    if (module == NULL) {
        if (verbose) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        lua_pushnil(L);
        return 1;
    }
    pushLua(L, module);   /* consumes the reference */
    return 1;
}

static int python_set(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python set", luaL_typename(L, -1));
    }
    PyObject* set = convertSetPython(L, -1);
    if (set == NULL) {
        return luapython_throw_convert(L, "luapython.set");
    }
    pushSetLua(L, set);
    return 1;
}

static int python_dict(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python dict", luaL_typename(L, -1));
    }
    PyObject* dict = convertDictPython(L, -1);
    if (dict == NULL) {
        return luapython_throw_convert(L, "luapython.dict");
    }
    pushDictLua(L, dict);
    return 1;
}

static int python_tuple(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python tuple", luaL_typename(L, -1));
    }
    PyObject* tuple = convertTuplePython(L, -1);
    if (tuple == NULL) {
        return luapython_throw_convert(L, "luapython.tuple");
    }
    pushTupleLua(L, tuple);
    return 1;
}

static int python_list(lua_State* L) {
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "Attempt to convert %s to python list", luaL_typename(L, -1));
    }
    PyObject* list = convertListPython(L, -1);
    if (list == NULL) {
        return luapython_throw_convert(L, "luapython.list");
    }
    pushListLua(L, list);
    return 1;
}

int python_tostring(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        return luaL_error(L, "python_tostring: not a Python object");
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    if (obj == NULL) {
        lua_pushliteral(L, "<finalized python object>");
        return 1;
    }
    if (obj == Py_None) {
        lua_pushliteral(L, "None");
        return 1;
    }
    PyObject* str = PyObject_Str(obj);
    if (str == NULL) {
        return luapython_throw(L, "python_tostring");
    }
    pushStringLua(L, str);   /* consumes */
    return 1;
}

int python_gc(lua_State* L) {
    if (lua_type(L, -1) != LUA_TUSERDATA) {
        return 0;
    }
    if (!Py_IsInitialized() || g_finalized) {
        /* Interpreter already finalized: the object is freed memory. Leaking
         * here is the only safe option, and the process is going away. */
        return 0;
    }
    PyObject** slot = (PyObject**)lua_touserdata(L, -1);
    if (slot == NULL || *slot == NULL) {
        return 0;
    }
    PyObject* obj = *slot;
    *slot = NULL;            /* a second __gc must not double free */
    Py_XDECREF(obj);
    return 0;
}

/* Shared __index implementation. Lua has a single indexing operator, so
 * `o.k` and `o[k]` are indistinguishable here and Python's getattr and
 * __getitem__ protocols have to be merged: try the attribute first, then the
 * item. Without the second step every object that only implements
 * __getitem__ (numpy arrays, range, os.environ, collections.deque, ...)
 * silently produced nil, which is both wrong and very hard to debug.
 *
 * A number key skips the attribute lookup: no attribute name can start with a
 * digit, so obj["1"] would fail anyway.
 *
 * "Not found" stays nil, as before: KeyError/IndexError/TypeError from the
 * item lookup mean the key is absent, not that the operation is broken. */
int luapython_index_shared(lua_State* L, PyObject* obj, int keyidx) {
    int key_type = lua_type(L, keyidx);
    PyObject* key = NULL;
    PyObject* value = NULL;
    const char* name = NULL;

    if (key_type == LUA_TSTRING) {
        size_t len;
        const char* text = lua_tolstring(L, keyidx, &len);
        name = text;
        key = PyUnicode_FromStringAndSize(text, (Py_ssize_t)len);
        if (key == NULL) {
            PyErr_Clear();
            lua_pushnil(L);
            return 1;
        }
        value = PyObject_GetAttr(obj, key);
        if (value == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
                Py_DECREF(key);
                return luapython_throw(L, name);
            }
            PyErr_Clear();           /* maybe it is a subscript after all */
        }
    } else if (key_type == LUA_TNUMBER) {
        /* Lua indices are 1-based, exactly like luapython.list: o[1] is the
         * first element. Negative indices keep Python's meaning so that
         * o[-1] is the last one. Passing the raw number through would make
         * range(10)[1] and list{...}[1] disagree by one. */
        lua_Number num = lua_tonumber(L, keyidx);
        if (num == 0) {
            lua_pushnil(L);          /* a Lua sequence has no element 0 */
            return 1;
        }
        if (num > 0) {
            num -= 1;
        }
        if (num > -9223372036854775808.0 && num < 9223372036854775808.0 &&
            num == (lua_Number)(long long)num) {
            key = PyLong_FromLongLong((long long)num);
        } else {
            key = PyFloat_FromDouble((double)num);
        }
        if (key == NULL) {
            PyErr_Clear();
            lua_pushnil(L);
            return 1;
        }
        name = "index";
    } else {
        return luaL_error(L, "attribute name must be a string, got %s",
                          luaL_typename(L, keyidx));
    }

    if (value == NULL) {
        value = PyObject_GetItem(obj, key);
        if (value == NULL) {
            if (PyErr_ExceptionMatches(PyExc_KeyError) ||
                PyErr_ExceptionMatches(PyExc_IndexError) ||
                PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_Clear();
                Py_DECREF(key);
                lua_pushnil(L);
                return 1;
            }
            Py_DECREF(key);
            return luapython_throw(L, name);
        }
    }
    Py_DECREF(key);
    pushLua(L, value);   /* consumes */
    return 1;
}

int python_index(lua_State* L) {
    if (!isPythonObject(L, -2)) {
        return luaL_error(L, "python_index: attempt to index a %s value", luaL_typename(L, -2));
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -2);
    if (obj == NULL) {
        return luaL_error(L, "python_index: the Python object is no longer valid");
    }
    return luapython_index_shared(L, obj, -1);
}

/* __eq shared by every container/object metatable. Without it Lua compares
 * userdata by reference, so two equal Python lists compared unequal. */
int python_object_eq(lua_State* L) {
    /* Lua 5.4+ runs __eq as soon as *one* operand carries it, so the other one
     * may be an unrelated full userdata (a FILE*, a socket, ...). Reading that
     * as a PyObject* would dereference whatever bytes it happens to hold, so
     * both sides must be verified first — same reason isPythonObject() refuses
     * a finalized interpreter. Anything that is not one of ours is simply not
     * equal, which is what Python answers for e.g. [] == 0. */
    if (!isPythonObject(L, -2) || !isPythonObject(L, -1)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    PyObject* left = *(PyObject**)lua_touserdata(L, -2);
    PyObject* right = *(PyObject**)lua_touserdata(L, -1);
    if (left == NULL || right == NULL) {
        lua_pushboolean(L, left == right);
        return 1;
    }
    int result = PyObject_RichCompareBool(left, right, Py_EQ);
    if (result < 0) {
        return luapython_throw(L, "eq (==)");
    }
    lua_pushboolean(L, result);
    return 1;
}

/* Render the exception (type/value as returned by PyErr_Fetch, normalized on
 * demand) into buf as "Name: message". Formatting failures are swallowed and
 * any error state they leave behind is cleared: this helper must not disturb
 * the caller's view of the interpreter. */
static void exception_text(char* buf, size_t size, PyObject* type, PyObject* value) {
    size_t used = 0;

    buf[0] = '\0';
    if (type != NULL) {
        PyObject* name = PyObject_GetAttrString(type, "__name__");
        if (name != NULL) {
            msg_append_unicode(buf, size, &used, name);
            Py_DECREF(name);
        }
    }
    if (value != NULL) {
        PyObject* text = PyObject_Str(value);
        if (text != NULL) {
            msg_appendf(buf, size, &used, ": ");
            msg_append_unicode(buf, size, &used, text);
            Py_DECREF(text);
        }
    }
    PyErr_Clear();
}

/* Render the pending Python exception into buf as "Name: message" and consume
 * it. Returns 0 when no exception is pending (buf is then empty). */
static int take_pending_exception(char* buf, size_t size) {
    PyObject *type = NULL, *value = NULL, *tb = NULL;

    buf[0] = '\0';
    if (!PyErr_Occurred()) {
        return 0;
    }
    PyErr_Fetch(&type, &value, &tb);
    PyErr_NormalizeException(&type, &value, &tb);
    exception_text(buf, size, type, value);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tb);
    return 1;
}

/* True when the pending exception means the object has no item-assignment
 * protocol at all, so falling back to setattr cannot hide a real item error.
 * AttributeError and the "not subscriptable" / "does not support item
 * assignment" TypeErrors that PyObject_SetItem raises on its own qualify;
 * every other TypeError (unhashable key, wrong value type, a real __setitem__
 * rejecting the entry, ...) is item semantics and must surface unchanged -
 * os.environ only accepts str values, so falling back on that TypeError would
 * silently turn `environ[k] = 5` into a corrupting setattr. The pending
 * exception is left exactly as found. */
static int setitem_unsupported(void) {
    char text[256];
    PyObject *type = NULL, *value = NULL, *tb = NULL;

    if (!PyErr_Occurred()) {
        return 0;
    }
    if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return 1;
    }
    if (!PyErr_ExceptionMatches(PyExc_TypeError)) {
        return 0;
    }
    PyErr_Fetch(&type, &value, &tb);
    PyErr_NormalizeException(&type, &value, &tb);
    exception_text(text, sizeof text, type, value);
    PyErr_Restore(type, value, tb);
    return strstr(text, "not subscriptable") != NULL ||
           strstr(text, "does not support item assignment") != NULL;
}

/* __newindex for generic Python objects. Lua has one assignment operator, so
 * `o.k = v` and `o[k] = v` are indistinguishable here and the write has to
 * merge Python's setattr and __setitem__ protocols. Unlike the read side
 * (which prefers getattr), the write is item-first: Lua's `o[k] = v` is meant
 * to mean Python's `o[k] = v`. The getattr-probe routing this replaces
 * silently corrupted every mapping that also carries a __dict__ (os.environ
 * in particular): `os.environ["get"] = "collide"` hit the MutableMapping.get
 * method, got redirected to setattr, never set the environment variable and
 * overwrote the method with a string, after which every os.getenv() died
 * with "TypeError: 'str' object is not callable".
 *
 * Routing:
 *   - numeric key: __setitem__ only (no attribute name can start with a
 *     digit, which is why the read side skips getattr for numbers too), with
 *     the very same 1-based mapping: Lua o[1] is element 0, o[-1] keeps
 *     Python's meaning. Lua has no element 0, so o[0] = v is an error instead
 *     of a silent no-op.
 *   - string key: __setitem__ first. When the object has no item-assignment
 *     protocol at all - the failure is AttributeError, or a TypeError saying
 *     the object is not subscriptable / does not support item assignment -
 *     clear it and fall back to setattr, so `obj.attr = v` keeps behaving
 *     like Python. Any other failure is item semantics (unhashable key, wrong
 *     value type, ...) and surfaces unchanged instead of being masked by a
 *     setattr attempt. When the fallback fails too, the error reports both
 *     the item-side exception and the setattr failure, so the final cause is
 *     visible.
 *
 * Deliberate asymmetry with the read side on objects that support both items
 * and attributes: the write lands in the item while the read prefers the
 * attribute, so `os.environ["get"] = "x"` sets the variable while
 * `os.environ["get"]` still reads the get() method. Use Python setattr to
 * write an attribute.
 *
 * Every failed probe clears its exception before the next attempt; nothing
 * may leak into the next Python call. */
int python_newindex(lua_State* L) {
    if (!isPythonObject(L, -3)) {
        return luaL_error(L, "python_newindex: attempt to index a %s value", luaL_typename(L, -3));
    }
    int key_type = lua_type(L, -2);
    if (key_type != LUA_TSTRING && key_type != LUA_TNUMBER) {
        return luaL_error(L, "python_newindex: attribute name must be a string, got %s",
                          luaL_typename(L, -2));
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -3);
    if (obj == NULL) {
        return luaL_error(L, "python_newindex: the Python object is no longer valid");
    }
    PyObject* value = convertPython(L, -1);
    if (value == NULL) {
        return luapython_throw_convert(L, "python_newindex: cannot convert value");
    }

    int result;
    if (key_type == LUA_TNUMBER) {
        /* Same 1-based mapping as luapython_index_shared(): o[1] writes the
         * element o[1] reads back, o[-1] the last one. */
        lua_Number num = lua_tonumber(L, -2);
        if (num == 0) {
            Py_DECREF(value);
            return luaL_error(L, "python_newindex: index 0 is out of range "
                              "(Lua subscripts are 1-based)");
        }
        if (num > 0) {
            num -= 1;
        }
        PyObject* key;
        if (num > -9223372036854775808.0 && num < 9223372036854775808.0 &&
            num == (lua_Number)(long long)num) {
            key = PyLong_FromLongLong((long long)num);
        } else {
            key = PyFloat_FromDouble((double)num);
        }
        if (key == NULL) {
            Py_DECREF(value);
            return luapython_throw_convert(L, "python_newindex: cannot convert index");
        }
        result = PyObject_SetItem(obj, key, value);
        Py_DECREF(key);
    } else {
        size_t len;
        const char* text = lua_tolstring(L, -2, &len);
        PyObject* key = PyUnicode_FromStringAndSize(text, (Py_ssize_t)len);
        if (key == NULL) {
            Py_DECREF(value);
            return luapython_throw_convert(L, "python_newindex: cannot convert key");
        }
        /* Item first (Python's o[k] = v), attribute only when the object
         * cannot do item assignment at all. */
        result = PyObject_SetItem(obj, key, value);
        if (result != 0 && setitem_unsupported()) {
            /* No item-assignment protocol: fall back to plain attribute
             * assignment. Stash the SetItem exception instead of clearing it
             * so a failed fallback can report both attempts. */
            PyObject *e_type = NULL, *e_value = NULL, *e_tb = NULL;
            PyErr_Fetch(&e_type, &e_value, &e_tb);
            result = PyObject_SetAttr(obj, key, value);
            if (result != 0) {
                char item_err[512];
                char attr_err[512];
                /* Consume the setattr failure first: exception_text() clears
                 * the interpreter's error state while formatting, which would
                 * wipe a still-pending exception. */
                take_pending_exception(attr_err, sizeof attr_err);
                PyErr_NormalizeException(&e_type, &e_value, &e_tb);
                exception_text(item_err, sizeof item_err, e_type, e_value);
                Py_XDECREF(e_type);
                Py_XDECREF(e_value);
                Py_XDECREF(e_tb);
                Py_DECREF(key);
                Py_DECREF(value);
                return luaL_error(L,
                    "python_newindex: %s (setattr fallback also failed: %s)",
                    item_err, attr_err);
            }
            Py_XDECREF(e_type);
            Py_XDECREF(e_value);
            Py_XDECREF(e_tb);
        }
        Py_DECREF(key);
    }
    Py_DECREF(value);
    if (result != 0) {
        return luapython_throw(L, "python_newindex");
    }
    return 0;
}

int python_addr(lua_State* L) {
    if (!isPythonObject(L, -1)) {
        return luaL_error(L, "python_addr: not a Python object");
    }
    PyObject* obj = *(PyObject**)lua_touserdata(L, -1);
    lua_pushfstring(L, "%p", obj);
    return 1;
}

int pushLua(lua_State* L, PyObject* obj) {
    if (obj == NULL || obj == Py_None) {
        /* None has no Lua equivalent: nil is the closest match. The reference
         * is still consumed, as documented, so callers do not have to special
         * case it (it is merely a no-op for immortal singletons on 3.12+). */
        Py_XDECREF(obj);
        lua_pushnil(L);
        return 1;
    }
    if (PyBool_Check(obj)) {
        lua_pushboolean(L, obj == Py_True);
        Py_DECREF(obj);
        return 1;
    }
    if (PyNumber_Check(obj)) {
        return pushNumberLua(L, obj);
    }
    if (PyUnicode_Check(obj)) {
        return pushStringLua(L, obj);
    }
    if (PySet_Check(obj)) {
        return pushSetLua(L, obj);
    }
    if (PyDict_Check(obj)) {
        return pushDictLua(L, obj);
    }
    if (PyTuple_Check(obj)) {
        return pushTupleLua(L, obj);
    }
    if (PyList_Check(obj)) {
        return pushListLua(L, obj);
    }
    if (PyModule_Check(obj)) {
        return pushModuleLua(L, obj);
    }
    if (PyIter_Check(obj)) {
        return pushIterLua(L, obj);
    }
    if (PyCallable_Check(obj)) {
        return pushFunctionLua(L, obj);
    }
    return pushClassLua(L, obj);
}

PyObject* luapython_number_to_py(lua_State* L, int index) {
    if (lua_type(L, index) != LUA_TNUMBER) {
        return NULL;
    }
#if LUA_VERSION_NUM >= 503
    if (lua_isinteger(L, index)) {
        return PyLong_FromLongLong((long long)lua_tointeger(L, index));
    }
#endif
    double value = lua_tonumber(L, index);
    /* Range check first: casting an out-of-range double to long long is
     * undefined behaviour, so it must never be evaluated unconditionally. */
    if (value >= -9223372036854775808.0 && value < 9223372036854775808.0 &&
        value == (double)(long long)value) {
        return PyLong_FromLongLong((long long)value);
    }
    return PyFloat_FromDouble(value);
}

PyObject* convertPythonAt(lua_State* L, int index, int depth) {
    if (depth > LUAPYTHON_MAX_NESTING) {
        /* A cyclic table would recurse until the C stack is exhausted and the
         * process dies in a segfault; report it as an ordinary Lua error.
         * Recorded, not raised: see luapython_convert_error(). */
        luapython_convert_error(L,
            "convertPython: table nested more than %d levels deep "
            "(cyclic table?)", LUAPYTHON_MAX_NESTING);
        return NULL;
    }
    /* Normalise to an absolute index: the table branch pushes values, and a
     * negative index would silently point somewhere else afterwards. */
    if (index < 0) {
        index = lua_gettop(L) + index + 1;
    }
    int type = lua_type(L, index);

    if (type == LUA_TUSERDATA) {
        if (!isPythonObject(L, index)) {
            luapython_convert_error(L, "convertPython: %s is not a Python object",
                                    luaL_typename(L, index));
            return NULL;
        }
        PyObject* obj = *(PyObject**)lua_touserdata(L, index);
        if (obj == NULL) {
            luapython_convert_error(L, "convertPython: the Python object is no longer valid");
            return NULL;
        }
        Py_XINCREF(obj);
        return obj;
    }
    if (type == LUA_TSTRING) {
        return convertStringPython(L, index);
    }
    if (type == LUA_TNUMBER) {
        return convertNumberPython(L, index);
    }
    if (type == LUA_TBOOLEAN) {
        PyObject* py_bool = lua_toboolean(L, index) ? Py_True : Py_False;
        Py_XINCREF(py_bool);
        return py_bool;
    }
    if (type == LUA_TNIL) {
        Py_XINCREF(Py_None);
        return Py_None;
    }
    if (type == LUA_TTABLE) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_should_convert_to_dict);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            loadTools(L);
            lua_rawgeti(L, LUA_REGISTRYINDEX, tools_should_convert_to_dict);
        }
        lua_pushvalue(L, index);
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            luapython_convert_error(L, "convertPython: shouldConvertToDict failed: %s",
                                    err != NULL ? err : "unknown error");
            lua_pop(L, 1);               /* the error object */
            return NULL;
        }
        int is_dict = lua_toboolean(L, -1);
        lua_pop(L, 1);
        if (is_dict) {
            return convertDictPythonAt(L, index, depth);
        }
        return convertListPythonAt(L, index, depth);
    }
    luapython_convert_error(L, "convertPython: unsupported Lua type %s",
                            luaL_typename(L, index));
    return NULL;
}

PyObject* convertPython(lua_State* L, int index) {
    return convertPythonAt(L, index, 0);
}

int luaopen_luapython_core(lua_State* L) {
    lua_createtable(L, 0, 10);

    /* import.lua turns the raw importer into the user facing one:
     * `wrap(python_import)` -> function(module_name). */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "luapython.import");
    lua_call(L, 1, 1);                 /* import module table */
    lua_pushcfunction(L, python_import);
    lua_call(L, 1, 1);                 /* adapted import function */
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
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        loadTools(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, tools_release_to_env);
    }
    lua_setfield(L, -2, "init");

    lua_pushcfunction(L, python_initialize);
    lua_setfield(L, -2, "initialize");
    lua_pushcfunction(L, python_finalize);
    lua_setfield(L, -2, "finalize");
    lua_pushcfunction(L, python_is_finalized);
    lua_setfield(L, -2, "isFinalized");
    lua_pushcfunction(L, python_addr);
    lua_setfield(L, -2, "addr");
    return 1;
}

int luaopen_luapython_core_import(lua_State* L) {
    lua_pushcfunction(L, python_import);
    return 1;
}
