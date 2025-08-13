#include "luapython.hpp"

int number_add(lua_State *L)
{
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_add: Attempt to add a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_add: Attempt to add on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        lua_pushnumber(L, a + b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1))
    {
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_add: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Add(py_a, py_b);
    if (!result)
    {        luaL_error(L, "number_add: Python addition failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_sub(lua_State *L)
{
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_sub: to subtract a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_sub: Attempt to subtract on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        lua_pushnumber(L, a - b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_sub: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Subtract(py_a, py_b);
    if (!result)
    {        
        luaL_error(L, "number_sub: Python subtraction failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_mul(lua_State *L)
{
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_mul: Attempt to multiply a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_mul: Attempt to multiply on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        lua_pushnumber(L, a * b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_mul: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Multiply(py_a, py_b);
    if (!result)
    {        
        luaL_error(L, "number_mul: Python multiplication failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_div(lua_State *L)
{
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_div: Attempt to divide a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_div: Attempt to divide on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        if (b == 0.0)
        {
            luaL_error(L, "number_div: Division by zero");
            return 0;
        }
        lua_pushnumber(L, a / b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_div: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_TrueDivide(py_a, py_b);
    if (!result)
    {        
        luaL_error(L, "number_div: Python division failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_mod(lua_State* L){
    if(!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_mod: Attempt to modulo a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {        luaL_error(L, "number_mod: Attempt to modulo on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        if (b == 0.0)
        {
            luaL_error(L, "number_mod: Modulo by zero");
            return 0;
        }
        lua_pushnumber(L, fmod(a, b));
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2))
    {
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_mod: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Remainder(py_a, py_b);
    if (!result)
    {
        luaL_error(L, "number_mod: Python modulo failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_pow(lua_State *L)
{
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_pow: Attempt to power a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_pow: Attempt to power on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        lua_pushnumber(L, pow(a, b));
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_pow: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Power(py_a, py_b, Py_None);
    if (!result)
    {        
        luaL_error(L, "number_pow: Python power failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_unm(lua_State *L)
{
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_unm: Attempt to negate a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(lua_isnumber(L, -1))
    {
        double a = lua_tonumber(L, -1);
        lua_pushnumber(L, -a);
        return 1;
    }
    PyObject* py_a = nullptr;
    bool release_a = false;
    if(lua_isnumber(L, -1))
    {
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if (!py_a)
    {
        luaL_error(L, "number_unm: Failed to convert Lua number to Python number");
        return 0;
    }
    PyObject* result = PyNumber_Negative(py_a);
    if (!result)
    {        
        luaL_error(L, "number_unm: Python negation failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    return 1;
}

int number_idiv(lua_State* L){
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_idiv: Attempt to integer divide a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_idiv: Attempt to integer divide on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        if (b == 0.0)
        {
            luaL_error(L, "number_idiv: Integer division by zero");
            return 0;
        }
        lua_pushinteger(L, static_cast<lua_Integer>(a / b));
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2))
    {
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_idiv: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_FloorDivide(py_a, py_b);
    if (!result)
    {        
        luaL_error(L, "number_idiv: Python integer division failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_band(lua_State* L){
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_band: Attempt to bitwise AND a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_band: Attempt to bitwise AND on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        lua_Integer a = lua_tointeger(L, -2);
        lua_Integer b = lua_tointeger(L, -1);
        lua_pushinteger(L, a & b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1))
    {
        double a = lua_tonumber(L, -1);
        py_a = PyFloat_FromDouble(a);
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        double b = lua_tonumber(L, -2);
        py_b = PyFloat_FromDouble(b);
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_band: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_And(py_a, py_b);
    if (!result)
    {        
        luaL_error(L, "number_band: Python bitwise AND failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_bor(lua_State* L){
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_bor: Attempt to bitwise OR a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_bor: Attempt to bitwise OR on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        lua_Integer a = lua_tointeger(L, -2);
        lua_Integer b = lua_tointeger(L, -1);
        lua_pushinteger(L, a | b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        py_b = PyFloat_FromDouble(lua_tonumber(L, -2));
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_bor: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Or(py_a, py_b);
    if (!result)
    {
        luaL_error(L, "number_bor: Python bitwise OR failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_bxor(lua_State* L){
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_bxor: Attempt to bitwise XOR a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_bxor: Attempt to bitwise XOR on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        lua_Integer a = lua_tointeger(L, -2);
        lua_Integer b = lua_tointeger(L, -1);
        lua_pushinteger(L, a ^ b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        py_b = PyFloat_FromDouble(lua_tonumber(L, -2));
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_bxor: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Xor(py_a, py_b);
    if (!result)
    {
        luaL_error(L, "number_bxor: Python bitwise XOR failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_bnot(lua_State* L){
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_bnot: Attempt to bitwise NOT a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(lua_isnumber(L, -1))
    {
        lua_Integer a = lua_tointeger(L, -1);
        lua_pushinteger(L, ~a);
        return 1;
    }
    PyObject* py_a = nullptr;
    bool release_a = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if (!py_a)
    {
        luaL_error(L, "number_bnot: Failed to convert Lua number to Python number");
        return 0;
    }
    PyObject* result = PyNumber_Invert(py_a);
    if (!result)
    {
        luaL_error(L, "number_bnot: Python bitwise NOT failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    return 1;
}

int number_shl(lua_State* L){
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_shl: Attempt to left shift a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_shl: Attempt to left shift on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        lua_Integer a = lua_tointeger(L, -2);
        lua_Integer b = lua_tointeger(L, -1);
        lua_pushinteger(L, a << b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        py_b = PyFloat_FromDouble(lua_tonumber(L, -2));
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_shl: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Lshift(py_b, py_a);
    if (!result)
    {
        luaL_error(L, "number_shl: Python left shift failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_shr(lua_State* L){
    if (!lua_isnumber(L, -1) && !lua_isuserdata(L, -1))
    {
        luaL_error(L, "number_shr: Attempt to right shift a %s value", luaL_typename(L, -1));
        return 0;
    }
    if(!lua_isnumber(L, -2) && !lua_isuserdata(L, -2))
    {
        luaL_error(L, "number_shr: Attempt to right shift on a %s value", luaL_typename(L, -2));
        return 0;
    }
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        lua_Integer a = lua_tointeger(L, -2);
        lua_Integer b = lua_tointeger(L, -1);
        lua_pushinteger(L, a >> b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        py_b = PyFloat_FromDouble(lua_tonumber(L, -2));
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_shr: Failed to convert Lua numbers to Python numbers");
        return 0;
    }
    PyObject* result = PyNumber_Rshift(py_b, py_a);
    if (!result)
    {
        luaL_error(L, "number_shr: Python right shift failed");
        return 0;
    }
    pushNumberLua(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_concat(lua_State* L){
    luaL_error(L, "number_concat: Attempt to concatenate on a %s value", luaL_typename(L, -2));
    return 0;
}

int number_len(lua_State* L){
    if(lua_isstring(L, -1))
    {
        size_t len;
        lua_tolstring(L, -1, &len);
        lua_pushinteger(L, len);
        return 1;
    }
    PyObject* py_a = nullptr;
    bool release_a = false;
    if(lua_isstring(L, -1)){
        py_a = PyUnicode_FromString(lua_tostring(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if (!py_a)
    {
        luaL_error(L, "number_len: Failed to convert Lua value to Python object");
        return 0;
    }
    Py_ssize_t result = PyObject_Length(py_a);
    if (result < 0)
    {
        luaL_error(L, "number_len: Python length failed");
        Py_DECREF(py_a);
        return 0;
    }
    lua_pushinteger(L, result);
    if(release_a)
        Py_DECREF(py_a);
    return 1;
}

int number_eq(lua_State* L){
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        lua_pushboolean(L, a == b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
        if(lua_isnumber(L, -2)){
        py_b = PyFloat_FromDouble(lua_tonumber(L, -2));
        release_b = true;
        }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
        }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_eq: Failed to convert Lua values to Python objects");
        return 0;
    }
    int result = PyObject_RichCompareBool(py_b, py_a, Py_EQ);
    lua_pushboolean(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_lt(lua_State* L){
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        lua_pushboolean(L, a < b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
    }
    if(lua_isnumber(L, -2)){
        py_b = PyFloat_FromDouble(lua_tonumber(L, -2));
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
    }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_lt: Failed to convert Lua values to Python objects");
        return 0;
    }
    int result = PyObject_RichCompareBool(py_b, py_a, Py_LT);
    lua_pushboolean(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_le(lua_State* L){
    if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
    {
        double a = lua_tonumber(L, -2);
        double b = lua_tonumber(L, -1);
        lua_pushboolean(L, a <= b);
        return 1;
    }
    PyObject* py_a = nullptr;
    PyObject* py_b = nullptr;
    bool release_a = false;
    bool release_b = false;
    if(lua_isnumber(L, -1)){
        py_a = PyFloat_FromDouble(lua_tonumber(L, -1));
        release_a = true;
    }else{
        py_a = *(PyObject**)lua_touserdata(L, -1);
        release_a = false;
        }
    if(lua_isnumber(L, -2)){
        py_b = PyFloat_FromDouble(lua_tonumber(L, -2));
        release_b = true;
    }else{
        py_b = *(PyObject**)lua_touserdata(L, -2);
        release_b = false;
        }
    if (!py_a || !py_b)
    {
        luaL_error(L, "number_le: Failed to convert Lua values to Python objects");
        return 0;
    }
    int result = PyObject_RichCompareBool(py_b, py_a, Py_LE);
    lua_pushboolean(L, result);
    if(release_a)
        Py_DECREF(py_a);
    if(release_b)
        Py_DECREF(py_b);
    return 1;
}

int number_tostring(lua_State* L){
    if(lua_isnumber(L, -1))
    {
        lua_pushstring(L, lua_tostring(L, -1));
        return 1;
    }
    PyObject* py_a = *(PyObject**)lua_touserdata(L, -1);
    if (!py_a)
    {
        luaL_error(L, "number_tostring: Failed to convert Lua value to Python object");
        return 0;
    }
    PyObject* result = PyObject_Str(py_a);
    if (!result)
    {
        luaL_error(L, "number_tostring: Python string conversion failed");
        return 0;
    }
    lua_pushstring(L, PyUnicode_AsUTF8(result));
    Py_DECREF(result);
    return 1;
}

int pushNumberLua(lua_State* L, PyObject* number) {
    if(!PyNumber_Check(number)){
        luaL_error(L, "pushNumberLua: Failed to set metatable for number");
        return 0;
    }
    if(PyLong_Check(number)){
        long number_long = PyLong_AsLong(number);
        if (!PyErr_Occurred()) {
            lua_pushinteger(L, PyLong_AsLong(number));
            return 1;
        }
    }else if(PyFloat_Check(number)){
        double number_double = PyFloat_AsDouble(number);
        if (!PyErr_Occurred()) {
            lua_pushnumber(L, number_double);
            return 1;
        }
    }
    void* userdata = lua_newuserdata(L, sizeof(PyObject*));
    *(PyObject**)userdata = number;
    Py_INCREF(number);
    lua_createtable(L, 0, 21);
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
    lua_setmetatable(L, -2);
    return 1;
}

PyObject* convertNumberPython(lua_State* L, int index){
    if(lua_isinteger(L, index)){
        return PyLong_FromLong(lua_tointeger(L, index));
    }
    return PyFloat_FromDouble(lua_tonumber(L, index));
}