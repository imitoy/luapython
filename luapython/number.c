#include "luapython.h"

#define isPythonNumber(L, index) (isPythonObject(L, index) && PyNumber_Check(*(PyObject**)lua_touserdata(L, index)))

/*
 * Operand handling for the arithmetic metamethods.
 *
 * Lua calls `a <op> b` metamethods with the left operand at -2 and the right
 * one at -1; getting this backwards silently computes `b <op> a`, which is
 * invisible for + and * but wrong for -, /, %, ** and for every comparison.
 *
 * Lua numbers are converted to PyLong when they are integral (so that bitwise
 * operators keep working) and to PyFloat otherwise.
 */
static PyObject* number_operand(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TNUMBER) {
        return luapython_number_to_py(L, index);
    }
    if (isPythonNumber(L, index)) {
        PyObject* obj = *(PyObject**)lua_touserdata(L, index);
        Py_XINCREF(obj);
        return obj;
    }
    return NULL;
}

static void binop_context(char* buf, size_t size, const char* name) {
    snprintf(buf, size, "number %s", name);
}

static int number_binop(lua_State* L, const char* name,
                        PyObject* (*op)(PyObject*, PyObject*)) {
    char context[64];
    binop_context(context, sizeof context, name);

    PyObject* left = number_operand(L, -2);
    PyObject* right = number_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "%s: operand must be a number, got %s",
                          context, luaL_typename(L, left == NULL ? -2 : -1));
    }
    PyObject* result = op(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    if (result == NULL) {
        return luapython_throw(L, context);
    }
    pushNumberLua(L, result);   /* consumes */
    return 1;
}

static int number_unop(lua_State* L, const char* name, PyObject* (*op)(PyObject*)) {
    char context[64];
    binop_context(context, sizeof context, name);

    PyObject* value = number_operand(L, -1);
    if (value == NULL) {
        return luaL_error(L, "%s: operand must be a number, got %s",
                          context, luaL_typename(L, -1));
    }
    PyObject* result = op(value);
    Py_DECREF(value);
    if (result == NULL) {
        return luapython_throw(L, context);
    }
    pushNumberLua(L, result);   /* consumes */
    return 1;
}

static int number_compare(lua_State* L, const char* name, int op) {
    char context[64];
    binop_context(context, sizeof context, name);

    PyObject* left = number_operand(L, -2);
    PyObject* right = number_operand(L, -1);
    if (left == NULL || right == NULL) {
        Py_XDECREF(left);
        Py_XDECREF(right);
        return luaL_error(L, "%s: operand must be a number, got %s",
                          context, luaL_typename(L, left == NULL ? -2 : -1));
    }
    int result = PyObject_RichCompareBool(left, right, op);
    Py_DECREF(left);
    Py_DECREF(right);
    if (result < 0) {
        return luapython_throw(L, context);
    }
    lua_pushboolean(L, result);
    return 1;
}

static PyObject* py_power(PyObject* a, PyObject* b) {
    return PyNumber_Power(a, b, Py_None);
}

int number_add(lua_State* L) {
    return number_binop(L, "add (+)", PyNumber_Add);
}

int number_sub(lua_State* L) {
    return number_binop(L, "sub (-)", PyNumber_Subtract);
}

int number_mul(lua_State* L) {
    return number_binop(L, "mul (*)", PyNumber_Multiply);
}

int number_div(lua_State* L) {
    return number_binop(L, "div (/)", PyNumber_TrueDivide);
}

int number_mod(lua_State* L) {
    return number_binop(L, "mod (%)", PyNumber_Remainder);
}

int number_pow(lua_State* L) {
    return number_binop(L, "pow (^)", py_power);
}

int number_idiv(lua_State* L) {
    return number_binop(L, "idiv (//)", PyNumber_FloorDivide);
}

int number_band(lua_State* L) {
    return number_binop(L, "and (&)", PyNumber_And);
}

int number_bor(lua_State* L) {
    return number_binop(L, "or (|)", PyNumber_Or);
}

int number_bxor(lua_State* L) {
    return number_binop(L, "xor (~)", PyNumber_Xor);
}

int number_shl(lua_State* L) {
    return number_binop(L, "shl (<<)", PyNumber_Lshift);
}

int number_shr(lua_State* L) {
    return number_binop(L, "shr (>>)", PyNumber_Rshift);
}

int number_unm(lua_State* L) {
    return number_unop(L, "unm (-)", PyNumber_Negative);
}

int number_bnot(lua_State* L) {
    return number_unop(L, "bnot (~)", PyNumber_Invert);
}

int number_eq(lua_State* L) {
    return number_compare(L, "eq (==)", Py_EQ);
}

int number_lt(lua_State* L) {
    return number_compare(L, "lt (<)", Py_LT);
}

int number_le(lua_State* L) {
    return number_compare(L, "le (<=)", Py_LE);
}

int number_concat(lua_State* L) {
    return luaL_error(L, "number_concat: attempt to concatenate a %s value",
                      luaL_typename(L, -2));
}

/* Lua 5.1 hands `__len` two arguments (the object and a nil), 5.2+ hands over
 * only one. Address the object as argument #1 so both work. */
int number_len(lua_State* L) {
    PyObject* value = number_operand(L, 1);
    if (value == NULL) {
        return luaL_error(L, "number_len: not a number");
    }
    Py_ssize_t len = PyObject_Length(value);
    Py_DECREF(value);
    if (len < 0) {
        return luapython_throw(L, "number len (#)");
    }
    lua_pushinteger(L, (lua_Integer)len);
    return 1;
}

int number_tostring(lua_State* L) {
    return python_tostring(L);
}

int pushNumberLua(lua_State* L, PyObject* obj) {
    if (!PyNumber_Check(obj)) {
        Py_XDECREF(obj);
        return luaL_error(L, "pushNumberLua: expected a Python number");
    }
    if (PyLong_Check(obj)) {
        long long value = PyLong_AsLongLong(obj);
        if (!PyErr_Occurred()) {
            lua_pushinteger(L, (lua_Integer)value);
            Py_DECREF(obj);        /* push* consumes the reference */
            return 1;
        }
        /* Too large for lua_Integer: clear and keep it as an object instead
         * of leaving an OverflowError behind. */
        PyErr_Clear();
    } else if (PyFloat_Check(obj)) {
        double value = PyFloat_AsDouble(obj);
        if (!PyErr_Occurred()) {
            lua_pushnumber(L, (lua_Number)value);
            Py_DECREF(obj);
            return 1;
        }
        PyErr_Clear();
    }

    void* point = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)point = obj;
    if (luaL_newmetatable(L, PYTHON_NUMBER_NAME)) {
        lua_pushcfunction(L, number_add);
        lua_setfield(L, -2, "__add");
        lua_pushcfunction(L, number_sub);
        lua_setfield(L, -2, "__sub");
        lua_pushcfunction(L, number_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, number_div);
        lua_setfield(L, -2, "__div");
        lua_pushcfunction(L, number_mod);
        lua_setfield(L, -2, "__mod");
        lua_pushcfunction(L, number_pow);
        lua_setfield(L, -2, "__pow");
        lua_pushcfunction(L, number_unm);
        lua_setfield(L, -2, "__unm");
        lua_pushcfunction(L, number_idiv);
        lua_setfield(L, -2, "__idiv");
        lua_pushcfunction(L, number_band);
        lua_setfield(L, -2, "__band");
        lua_pushcfunction(L, number_bor);
        lua_setfield(L, -2, "__bor");
        lua_pushcfunction(L, number_bxor);
        lua_setfield(L, -2, "__bxor");
        lua_pushcfunction(L, number_bnot);
        lua_setfield(L, -2, "__bnot");
        lua_pushcfunction(L, number_shl);
        lua_setfield(L, -2, "__shl");
        lua_pushcfunction(L, number_shr);
        lua_setfield(L, -2, "__shr");
        lua_pushcfunction(L, number_concat);
        lua_setfield(L, -2, "__concat");
        lua_pushcfunction(L, number_len);
        lua_setfield(L, -2, "__len");
        lua_pushcfunction(L, number_eq);
        lua_setfield(L, -2, "__eq");
        lua_pushcfunction(L, number_lt);
        lua_setfield(L, -2, "__lt");
        lua_pushcfunction(L, number_le);
        lua_setfield(L, -2, "__le");
        lua_pushcfunction(L, number_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, python_gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, python_index);
        lua_setfield(L, -2, "__index");
        luapython_register_metatable(L);
    }
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertNumberPython(lua_State* L, int index) {
    if (lua_type(L, index) != LUA_TNUMBER) {
        luaL_error(L, "convertNumberPython: number expected, got %s", luaL_typename(L, index));
        return NULL;
    }
    return luapython_number_to_py(L, index);
}
