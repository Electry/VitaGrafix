#include <math.h>
#include <string.h>

#include "interpreter.h"
#include "parser.h"
#include "op.h"

inline int min(int a, int b) { return a < b ? a : b; }
inline int max(int a, int b) { return a > b ? a : b; }

#define __math_do_fn_op_unary(lhs, fn)\
    bool ret = true;\
{\
    switch (lhs->type) {\
        case DATA_TYPE_SIGNED:   lhs->data.int32  = (int32_t)fn(lhs->data.int32); break;\
        case DATA_TYPE_UNSIGNED: lhs->data.uint32 = (uint32_t)fn(lhs->data.uint32); break;\
        case DATA_TYPE_FLOAT:    lhs->data.fl32   = (float)fn(lhs->data.fl32); break;\
        default:                 ret = false; break;\
    }\
}

#define __math_do_fn_op_binary(lhs, rhs, fn)\
    bool ret = true;\
{\
    common_cast(lhs, rhs);\
    switch (lhs->type) {\
        case DATA_TYPE_SIGNED:   lhs->data.int32  = (int32_t)fn(lhs->data.int32, rhs->data.int32); break;\
        case DATA_TYPE_UNSIGNED: lhs->data.uint32 = (uint32_t)fn(lhs->data.uint32, rhs->data.uint32); break;\
        case DATA_TYPE_FLOAT:    lhs->data.fl32   = (float)fn(lhs->data.fl32, rhs->data.fl32); break;\
        default:                 ret = false; break;\
    }\
}

#define __math_do_infix_op(lhs, rhs, op)\
    bool ret = true;\
{\
    if (#op[0] == '-' && rhs->type == DATA_TYPE_UNSIGNED && rhs->data.uint32 > lhs->data.uint32)\
        value_cast(lhs, DATA_TYPE_SIGNED);\
    common_cast(lhs, rhs);\
    switch (lhs->type) {\
        case DATA_TYPE_SIGNED:   lhs->data.int32  op##= rhs->data.int32; break;\
        case DATA_TYPE_UNSIGNED: lhs->data.uint32 op##= rhs->data.uint32; break;\
        case DATA_TYPE_FLOAT:    lhs->data.fl32   op##= rhs->data.fl32; break;\
        default:                 ret = false; break;\
    }\
}

#define __math_do_infix_op_bw(lhs, rhs, op)\
    bool ret = true;\
{\
    switch (lhs->type) {\
        case DATA_TYPE_SIGNED:\
        case DATA_TYPE_FLOAT:\
            lhs->data.uint32 op##= rhs->data.uint32;\
            break;\
        case DATA_TYPE_UNSIGNED:\
            lhs->data.int32 op##= rhs->data.uint32;\
            break;\
        default: ret = false; break;\
    }\
}

#define __math_do_const(lhs, val)\
    bool ret = true;\
{\
    lhs->size = 4;\
    lhs->type = DATA_TYPE_FLOAT;\
    lhs->data.fl32 = val;\
}

bool op_math_add(value_t *lhs, value_t *rhs)         { __math_do_infix_op(lhs, rhs, +); return ret; }
bool op_math_subtract(value_t *lhs, value_t *rhs)    { __math_do_infix_op(lhs, rhs, -); return ret; }
bool op_math_multiply(value_t *lhs, value_t *rhs)    {
    // Handle special case where RAW is multiplied by UINT
    //  aka. raw repeat
    if ((lhs->type == DATA_TYPE_RAW
            && rhs->type == DATA_TYPE_UNSIGNED
            && rhs->data.uint32 > 0)
                || (rhs->type == DATA_TYPE_RAW
                    && lhs->type == DATA_TYPE_UNSIGNED
                    && lhs->data.uint32 > 0)) {
        // Allow both UINT * RAW, and RAW * UINT
        value_t *rpt = lhs->type == DATA_TYPE_RAW ? lhs : rhs;
        uint32_t cnt = lhs->type == DATA_TYPE_RAW ? rhs->data.uint32 : lhs->data.uint32;
        bool ret = true;

        // We need to keep the original as LHS/RHS is gonna get larger each iteration
        value_t orig;
        memcpy(&orig, rpt, sizeof(value_t));

        // Do concats
        for (uint32_t i = 0; i < cnt - 1; i++) {
            ret = op_datatype_raw_concat(rpt, &orig);
            if (!ret)
                break;
        }

        // If RHS was repeated (concatted), copy the result to LHS
        if (rhs->type == DATA_TYPE_RAW)
            memcpy(lhs, rhs, sizeof(value_t));

        return ret;
    // Continue as usual
    } else {
        __math_do_infix_op(lhs, rhs, *);
        return ret;
    }
}
bool op_math_divide(value_t *lhs, value_t *rhs)      { __math_do_infix_op(lhs, rhs, /); return ret; }
bool op_math_modulo(value_t *lhs, value_t *rhs) {
    common_cast(lhs, rhs);
    switch (lhs->type) {
        case DATA_TYPE_SIGNED:   lhs->data.int32 %= rhs->data.int32; break;
        case DATA_TYPE_UNSIGNED: lhs->data.uint32 %= rhs->data.uint32; break;
        case DATA_TYPE_FLOAT:
        default:                 return false;
    }
    return true;
}

bool op_math_bitwise_or(value_t *lhs, value_t *rhs)  { __math_do_infix_op_bw(lhs, rhs, |); return ret; }
bool op_math_bitwise_xor(value_t *lhs, value_t *rhs) { __math_do_infix_op_bw(lhs, rhs, ^); return ret; }
bool op_math_bitwise_and(value_t *lhs, value_t *rhs) { __math_do_infix_op_bw(lhs, rhs, &); return ret; }
bool op_math_bitwise_l(value_t *lhs, value_t *rhs)   { __math_do_infix_op_bw(lhs, rhs, <<); return ret; }
bool op_math_bitwise_r(value_t *lhs, value_t *rhs)   { __math_do_infix_op_bw(lhs, rhs, >>); return ret; }

bool op_math_fn_abs(value_t *lhs)                    { __math_do_fn_op_unary(lhs, fabs); return ret; }
bool op_math_fn_acos(value_t *lhs)                   { __math_do_fn_op_unary(lhs, acos); return ret; }
bool op_math_fn_align(value_t *lhs, value_t *rhs)    {
    common_cast(lhs, rhs);
    __math_do_fn_op_unary(rhs, fabs); // make rhs absolute
    if (!ret) return ret;
    switch (lhs->type) {
        case DATA_TYPE_SIGNED:
            lhs->data.int32 = lhs->data.int32 + rhs->data.int32 - 1;
            lhs->data.int32 -= (rhs->data.int32 + (lhs->data.int32 % rhs->data.int32)) % rhs->data.int32;
            break;
        case DATA_TYPE_UNSIGNED: lhs->data.uint32 = (uint32_t)(lhs->data.uint32 + rhs->data.uint32 - 1) / rhs->data.uint32 * rhs->data.uint32; break;
        case DATA_TYPE_FLOAT:    lhs->data.fl32   = (float)(ceil(lhs->data.fl32 / rhs->data.fl32) * rhs->data.fl32); break;
        default:                 return false;
    }
    return true;
}
bool op_math_fn_asin(value_t *lhs)                   { __math_do_fn_op_unary(lhs, asin); return ret; }
bool op_math_fn_atan2(value_t *lhs, value_t *rhs)    { __math_do_fn_op_binary(lhs, rhs, atan2); return ret; }
bool op_math_fn_atan(value_t *lhs)                   { __math_do_fn_op_unary(lhs, atan); return ret; }
bool op_math_fn_ceil(value_t *lhs)                   { __math_do_fn_op_unary(lhs, ceil); return ret; }
bool op_math_fn_cosh(value_t *lhs)                   { __math_do_fn_op_unary(lhs, cosh); return ret; }
bool op_math_fn_cos(value_t *lhs)                    { __math_do_fn_op_unary(lhs, cos); return ret; }
bool op_math_fn_exp(value_t *lhs)                    { __math_do_fn_op_unary(lhs, exp); return ret; }
bool op_math_fn_floor(value_t *lhs)                  { __math_do_fn_op_unary(lhs, floor); return ret; }
bool op_math_fn_ln(value_t *lhs)                     { __math_do_fn_op_unary(lhs, log); return ret; }
bool op_math_fn_log10(value_t *lhs)                  { __math_do_fn_op_unary(lhs, log10); return ret; }
bool op_math_fn_min(value_t *lhs, value_t *rhs)      {
    common_cast(lhs, rhs);
    switch (lhs->type) {
        case DATA_TYPE_SIGNED:   lhs->data.int32  = lhs->data.int32 < rhs->data.int32 ? lhs->data.int32 : rhs->data.int32; break;
        case DATA_TYPE_UNSIGNED: lhs->data.uint32 = lhs->data.uint32 < rhs->data.uint32 ? lhs->data.uint32 : rhs->data.uint32; break;
        case DATA_TYPE_FLOAT:    lhs->data.fl32   = lhs->data.fl32 < rhs->data.fl32 ? lhs->data.fl32 : rhs->data.fl32; break;
        default:                 return false;
    }
    return true;
}
bool op_math_fn_max(value_t *lhs, value_t *rhs)      {
    common_cast(lhs, rhs);
    switch (lhs->type) {
        case DATA_TYPE_SIGNED:   lhs->data.int32  = lhs->data.int32 > rhs->data.int32 ? lhs->data.int32 : rhs->data.int32; break;
        case DATA_TYPE_UNSIGNED: lhs->data.uint32 = lhs->data.uint32 > rhs->data.uint32 ? lhs->data.uint32 : rhs->data.uint32; break;
        case DATA_TYPE_FLOAT:    lhs->data.fl32   = lhs->data.fl32 > rhs->data.fl32 ? lhs->data.fl32 : rhs->data.fl32; break;
        default:                 return false;
    }
    return true;
}
bool op_math_fn_pow(value_t *lhs, value_t *rhs)      { __math_do_fn_op_binary(lhs, rhs, pow); return ret; }
bool op_math_fn_sinh(value_t *lhs)                   { __math_do_fn_op_unary(lhs, sinh); return ret; }
bool op_math_fn_sin(value_t *lhs)                    { __math_do_fn_op_unary(lhs, sin); return ret; }
bool op_math_fn_sqrt(value_t *lhs)                   { __math_do_fn_op_unary(lhs, sqrt); return ret; }
bool op_math_fn_tanh(value_t *lhs)                   { __math_do_fn_op_unary(lhs, tanh); return ret; }
bool op_math_fn_tan(value_t *lhs)                    { __math_do_fn_op_unary(lhs, tan); return ret; }

bool op_math_const_pi(value_t *lhs)                  { __math_do_const(lhs, 3.14159265358979323846f); return ret; }
bool op_math_const_e(value_t *lhs)                   { __math_do_const(lhs, 2.71828182845904523536f); return ret; }
