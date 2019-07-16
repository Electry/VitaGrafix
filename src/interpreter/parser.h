#ifndef PARSER_H
#define PARSER_H
#include <stdbool.h>

typedef intp_value_t           value_t;
typedef intp_value_data_type_t value_data_type_t;

#define MAX_FN_ARGS 4

#define TOKEN_GET_ARGN(token)                     \
    (((token)->flags & TOKEN_ARITY_4) ? 4 :       \
    (((token)->flags & TOKEN_ARITY_3) ? 3 :       \
    (((token)->flags & TOKEN_ARITY_2) ? 2 :       \
     ((token)->flags & TOKEN_ARITY_1  ? 1 : 0))))

#define TOKEN_ARITY_1   (1 << 0)
#define TOKEN_ARITY_2   (1 << 1)
#define TOKEN_ARITY_3   (1 << 2)
#define TOKEN_ARITY_4   (1 << 3)

#define TOKEN_FORCE_RAW (1 << 5)
#define TOKEN_CONSTANT  (1 << 6)
#define TOKEN_INFIX     (1 << 7)

typedef enum {
    TOKEN_PRIMITIVE = 0,

    TOKEN_ADD,
    TOKEN_SUBTRACT,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,

    TOKEN_BITWISE_OR,
    TOKEN_BITWISE_XOR,
    TOKEN_BITWISE_AND,
    TOKEN_BITWISE_L,
    TOKEN_BITWISE_R,

    TOKEN_BRACKET_OPEN,
    TOKEN_BRACKET_CLOSE,
    TOKEN_ARGUMENT_SEP,

    TOKEN_FN_ABS,
    TOKEN_FN_ACOS,
    TOKEN_FN_ALIGN,
    TOKEN_FN_ASIN,
    TOKEN_FN_ATAN2,
    TOKEN_FN_ATAN,
    TOKEN_FN_CEIL,
    TOKEN_FN_COSH,
    TOKEN_FN_COS,
    TOKEN_FN_EXP,
    TOKEN_FN_FLOOR,
    TOKEN_FN_LN,
    TOKEN_FN_LOG10,
    TOKEN_FN_MIN,
    TOKEN_FN_MAX,
    TOKEN_FN_POW,
    TOKEN_FN_SINH,
    TOKEN_FN_SIN,
    TOKEN_FN_SQRT,
    TOKEN_FN_TANH,
    TOKEN_FN_TAN,

    TOKEN_RAW_INT8,
    TOKEN_RAW_INT16,
    TOKEN_RAW_INT32,
    TOKEN_RAW_UINT8,
    TOKEN_RAW_UINT16,
    TOKEN_RAW_UINT32,
    TOKEN_RAW_FL32,
    TOKEN_RAW_BYTES,

    TOKEN_RAW_CONCAT,

    TOKEN_CAST_INT,
    TOKEN_CAST_UINT,
    TOKEN_CAST_FLOAT,

    TOKEN_CONST_PI,
    TOKEN_CONST_E,

    TOKEN_VG_CONFIG_FB_WIDTH,
    TOKEN_VG_CONFIG_FB_HEIGHT,
    TOKEN_VG_CONFIG_IB_WIDTH,
    TOKEN_VG_CONFIG_IB_HEIGHT,
    TOKEN_VG_CONFIG_VBLANK,
    TOKEN_VG_CONFIG_MSAA,

    TOKEN_ENCODE_T1_MOVT,
    TOKEN_ENCODE_T1_MOV,
    TOKEN_ENCODE_T2_MOV,
    TOKEN_ENCODE_T3_MOV,
    TOKEN_ENCODE_A1_MOV,
    TOKEN_ENCODE_A2_MOV,
    TOKEN_ENCODE_BKPT,
    TOKEN_ENCODE_NOP,

#ifdef BUILD_LEGACY_SUPPORT
    TOKEN_LEGACY,
#endif
    TOKEN_TERMINATOR,

    TOKEN_INVALID
} token_type_t;

typedef struct {
    int precedence;
    const char *string;
    token_type_t type;
    byte_t flags;

    bool *op;
} token_t;

//printf("%s returning code: %d [%d]\n", __func__, code, pos);

#define __intp_ret_status(code, pos) {\
    intp_status_t ret = {code, pos};\
    return ret;\
}

void value_raw(value_t *lhs, int size);
void value_cast(value_t *lhs, value_data_type_t data_type);
void common_cast(value_t *lhs, value_t *rhs);
intp_status_t parse_value(const char *expr, uint32_t *pos, value_t *value, bool force_raw, bool allow_brckt_next);
intp_status_t parse_expression(const char *expr, uint32_t *pos, value_t *value, bool force_raw, bool allow_brckt_next);

#endif
