#include <string.h>

#include "interpreter.h"
#include "parser.h"
#include "op.h"

#define __datatype_raw_cast_op(lhs, tp, sz) {\
    value_cast(lhs, tp);\
    value_raw(lhs, sz);\
}

bool op_datatype_raw_int8(value_t *lhs)   { __datatype_raw_cast_op(lhs, DATA_TYPE_SIGNED, 1); return true; }
bool op_datatype_raw_int16(value_t *lhs)  { __datatype_raw_cast_op(lhs, DATA_TYPE_SIGNED, 2); return true; }
bool op_datatype_raw_int32(value_t *lhs)  { __datatype_raw_cast_op(lhs, DATA_TYPE_SIGNED, 4); return true; }
bool op_datatype_raw_uint8(value_t *lhs)  { __datatype_raw_cast_op(lhs, DATA_TYPE_UNSIGNED, 1); return true; }
bool op_datatype_raw_uint16(value_t *lhs) { __datatype_raw_cast_op(lhs, DATA_TYPE_UNSIGNED, 2); return true; }
bool op_datatype_raw_uint32(value_t *lhs) { __datatype_raw_cast_op(lhs, DATA_TYPE_UNSIGNED, 4); return true; }
bool op_datatype_raw_fl32(value_t *lhs)   { __datatype_raw_cast_op(lhs, DATA_TYPE_FLOAT, 4); return true; }
bool op_datatype_raw_bytes(value_t *lhs)  { __datatype_raw_cast_op(lhs, DATA_TYPE_RAW, lhs->size); return true; }

bool op_datatype_raw_concat(value_t *lhs, value_t *rhs) {
    if (lhs->size + rhs->size > MAX_VALUE_SIZE) {
        // Won't fit :(
        return false;
    }

    value_raw(lhs, lhs->size);
    value_raw(rhs, rhs->size);
    
    memcpy(&lhs->data.raw[lhs->size], rhs->data.raw, rhs->size);
    lhs->size += rhs->size;
    return true;
}

bool op_datatype_cast_int(value_t *lhs)   { value_cast(lhs, DATA_TYPE_SIGNED); return true; }
bool op_datatype_cast_uint(value_t *lhs)  { value_cast(lhs, DATA_TYPE_UNSIGNED); return true; }
bool op_datatype_cast_float(value_t *lhs) { value_cast(lhs, DATA_TYPE_FLOAT); return true; }