#ifndef OP_H
#define OP_H
#include <stdbool.h>

bool op_math_add(value_t *lhs, value_t *rhs);
bool op_math_subtract(value_t *lhs, value_t *rhs);
bool op_math_multiply(value_t *lhs, value_t *rhs);
bool op_math_divide(value_t *lhs, value_t *rhs);

bool op_math_bitwise_or(value_t *lhs, value_t *rhs);
bool op_math_bitwise_xor(value_t *lhs, value_t *rhs);
bool op_math_bitwise_and(value_t *lhs, value_t *rhs);
bool op_math_bitwise_l(value_t *lhs, value_t *rhs);
bool op_math_bitwise_r(value_t *lhs, value_t *rhs);

bool op_math_fn_abs(value_t *lhs);
bool op_math_fn_acos(value_t *lhs);
bool op_math_fn_align(value_t *lhs, value_t *rhs);
bool op_math_fn_asin(value_t *lhs);
bool op_math_fn_atan2(value_t *lhs, value_t *rhs);
bool op_math_fn_atan(value_t *lhs);
bool op_math_fn_ceil(value_t *lhs);
bool op_math_fn_cosh(value_t *lhs);
bool op_math_fn_cos(value_t *lhs);
bool op_math_fn_exp(value_t *lhs);
bool op_math_fn_floor(value_t *lhs);
bool op_math_fn_ln(value_t *lhs);
bool op_math_fn_log10(value_t *lhs);
bool op_math_fn_min(value_t *lhs, value_t *rhs);
bool op_math_fn_max(value_t *lhs, value_t *rhs);
bool op_math_fn_pow(value_t *lhs, value_t *rhs);
bool op_math_fn_sinh(value_t *lhs);
bool op_math_fn_sin(value_t *lhs);
bool op_math_fn_sqrt(value_t *lhs);
bool op_math_fn_tanh(value_t *lhs);
bool op_math_fn_tan(value_t *lhs);

bool op_math_const_pi(value_t *lhs);
bool op_math_const_e(value_t *lhs);

bool op_datatype_raw_int8(value_t *lhs);
bool op_datatype_raw_int16(value_t *lhs);
bool op_datatype_raw_int32(value_t *lhs);
bool op_datatype_raw_uint8(value_t *lhs);
bool op_datatype_raw_uint16(value_t *lhs);
bool op_datatype_raw_uint32(value_t *lhs);
bool op_datatype_raw_fl32(value_t *lhs);
bool op_datatype_raw_bytes(value_t *lhs);

bool op_datatype_raw_concat(value_t *lhs, value_t *rhs);

bool op_datatype_cast_int(value_t *lhs);
bool op_datatype_cast_uint(value_t *lhs);
bool op_datatype_cast_float(value_t *lhs);

bool op_encode_t1_mov(value_t *out, value_t *value);
bool op_encode_t2_mov(value_t *out, value_t *setflags, value_t *value);
bool op_encode_t3_mov(value_t *out, value_t *value);
bool op_encode_t1_movt(value_t *out, value_t *value);
bool op_encode_t2_vmov_f32(value_t *out, value_t *value);
bool op_encode_a1_mov(value_t *out, value_t *setflags, value_t *value);
bool op_encode_a2_mov(value_t *out, value_t *value);
bool op_encode_bkpt(value_t *out);
bool op_encode_nop(value_t *out);
bool op_encode_unk(value_t *out);

bool op_vg_config_fb_width(value_t *out);
bool op_vg_config_fb_height(value_t *out);
bool op_vg_config_ib_width(value_t *out);
bool op_vg_config_ib_height(value_t *out);
bool op_vg_config_vblank(value_t *out);
bool op_vg_config_msaa(value_t *out);

#endif
