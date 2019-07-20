#include <stdio.h>
#include <stdlib.h>
#ifndef BUILD_VITAGRAFIX
#include <ctype.h>
#endif
#include <string.h>

#include "interpreter.h"
#include "parser.h"

#ifdef BUILD_LEGACY_SUPPORT
#include "legacy.h"
#endif

#include "op.h"


#define TOKEN(type) &_TOKENS[type]
const token_t _TOKENS[TOKEN_INVALID + 1] = {
    {90, NULL,      TOKEN_PRIMITIVE,      0,              NULL},

    {10, "+",       TOKEN_ADD,            TOKEN_INFIX,    (bool *)op_math_add},
    {10, "-",       TOKEN_SUBTRACT,       TOKEN_INFIX,    (bool *)op_math_subtract},
    {11, "*",       TOKEN_MULTIPLY,       TOKEN_INFIX,    (bool *)op_math_multiply},
    {11, "/",       TOKEN_DIVIDE,         TOKEN_INFIX,    (bool *)op_math_divide},

    {6,  "|",       TOKEN_BITWISE_OR,     TOKEN_INFIX,    (bool *)op_math_bitwise_or},
    {7,  "^",       TOKEN_BITWISE_XOR,    TOKEN_INFIX,    (bool *)op_math_bitwise_xor},
    {8,  "&",       TOKEN_BITWISE_AND,    TOKEN_INFIX,    (bool *)op_math_bitwise_and},
    {9,  "<<",      TOKEN_BITWISE_L,      TOKEN_INFIX,    (bool *)op_math_bitwise_l},
    {9,  ">>",      TOKEN_BITWISE_R,      TOKEN_INFIX,    (bool *)op_math_bitwise_r},

    {20, "(",       TOKEN_BRACKET_OPEN,   0,              NULL},
    {20, ")",       TOKEN_BRACKET_CLOSE,  0,              NULL},
    {20, ",",       TOKEN_ARGUMENT_SEP,   0,              NULL},

    {50, "abs",     TOKEN_FN_ABS,         TOKEN_ARITY_1,  (bool *)op_math_fn_abs},
    {50, "acos",    TOKEN_FN_ACOS,        TOKEN_ARITY_1,  (bool *)op_math_fn_acos},
    {50, "align",   TOKEN_FN_ALIGN,       TOKEN_ARITY_2,  (bool *)op_math_fn_align},
    {50, "asin",    TOKEN_FN_ASIN,        TOKEN_ARITY_1,  (bool *)op_math_fn_asin},
    {50, "atan2",   TOKEN_FN_ATAN2,       TOKEN_ARITY_2,  (bool *)op_math_fn_atan2},
    {50, "atan",    TOKEN_FN_ATAN,        TOKEN_ARITY_1,  (bool *)op_math_fn_atan},
    {50, "ceil",    TOKEN_FN_CEIL,        TOKEN_ARITY_1,  (bool *)op_math_fn_ceil},
    {50, "cosh",    TOKEN_FN_COSH,        TOKEN_ARITY_1,  (bool *)op_math_fn_cosh},
    {50, "cos",     TOKEN_FN_COS,         TOKEN_ARITY_1,  (bool *)op_math_fn_cos},
    {50, "exp",     TOKEN_FN_EXP,         TOKEN_ARITY_1,  (bool *)op_math_fn_exp},
    {50, "floor",   TOKEN_FN_FLOOR,       TOKEN_ARITY_1,  (bool *)op_math_fn_floor},
    {50, "ln",      TOKEN_FN_LN,          TOKEN_ARITY_1,  (bool *)op_math_fn_ln},
    {50, "log10",   TOKEN_FN_LOG10,       TOKEN_ARITY_1,  (bool *)op_math_fn_log10},
    {50, "min",     TOKEN_FN_MIN,         TOKEN_ARITY_2,  (bool *)op_math_fn_min},
    {50, "max",     TOKEN_FN_MAX,         TOKEN_ARITY_2,  (bool *)op_math_fn_max},
    {50, "pow",     TOKEN_FN_POW,         TOKEN_ARITY_2,  (bool *)op_math_fn_pow},
    {50, "sinh",    TOKEN_FN_SINH,        TOKEN_ARITY_1,  (bool *)op_math_fn_sinh},
    {50, "sin",     TOKEN_FN_SIN,         TOKEN_ARITY_1,  (bool *)op_math_fn_sin},
    {50, "sqrt",    TOKEN_FN_SQRT,        TOKEN_ARITY_1,  (bool *)op_math_fn_sqrt},
    {50, "tanh",    TOKEN_FN_TANH,        TOKEN_ARITY_1,  (bool *)op_math_fn_tanh},
    {50, "tan",     TOKEN_FN_TAN,         TOKEN_ARITY_1,  (bool *)op_math_fn_tan},

    {52, "int8",    TOKEN_RAW_INT8,       TOKEN_ARITY_1,  (bool *)op_datatype_raw_int8},
    {52, "int16",   TOKEN_RAW_INT16,      TOKEN_ARITY_1,  (bool *)op_datatype_raw_int16},
    {52, "int32",   TOKEN_RAW_INT32,      TOKEN_ARITY_1,  (bool *)op_datatype_raw_int32},
    {52, "uint8",   TOKEN_RAW_UINT8,      TOKEN_ARITY_1,  (bool *)op_datatype_raw_uint8},
    {52, "uint16",  TOKEN_RAW_UINT16,     TOKEN_ARITY_1,  (bool *)op_datatype_raw_uint16},
    {52, "uint32",  TOKEN_RAW_UINT32,     TOKEN_ARITY_1,  (bool *)op_datatype_raw_uint32},
    {52, "fl32",    TOKEN_RAW_FL32,       TOKEN_ARITY_1,  (bool *)op_datatype_raw_fl32},
    {52, "bytes",   TOKEN_RAW_BYTES,      TOKEN_ARITY_1 | TOKEN_FORCE_RAW,  (bool *)op_datatype_raw_bytes},

    {5,  ".",       TOKEN_RAW_CONCAT,     TOKEN_INFIX,    (bool *)op_datatype_raw_concat},

    {51, "int",     TOKEN_CAST_INT,       TOKEN_ARITY_1,  (bool *)op_datatype_cast_int},
    {51, "uint",    TOKEN_CAST_UINT,      TOKEN_ARITY_1,  (bool *)op_datatype_cast_uint},
    {51, "float",   TOKEN_CAST_FLOAT,     TOKEN_ARITY_1,  (bool *)op_datatype_cast_float},

    {90, "pi",      TOKEN_CONST_PI,       TOKEN_CONSTANT, (bool *)op_math_const_pi},
    {90, "e",       TOKEN_CONST_E,        TOKEN_CONSTANT, (bool *)op_math_const_e},

    {90, "fb_w",    TOKEN_VG_CONFIG_FB_WIDTH,  TOKEN_CONSTANT, (bool *)op_vg_config_fb_width},
    {90, "fb_h",    TOKEN_VG_CONFIG_FB_HEIGHT, TOKEN_CONSTANT, (bool *)op_vg_config_fb_height},
    {90, "ib_w",    TOKEN_VG_CONFIG_IB_WIDTH,  TOKEN_ARITY_1,  (bool *)op_vg_config_ib_width},
    {90, "ib_h",    TOKEN_VG_CONFIG_IB_HEIGHT, TOKEN_ARITY_1,  (bool *)op_vg_config_ib_height},
    {90, "vblank",  TOKEN_VG_CONFIG_VBLANK,    TOKEN_CONSTANT, (bool *)op_vg_config_vblank},
    {90, "msaa",    TOKEN_VG_CONFIG_MSAA,      TOKEN_CONSTANT, (bool *)op_vg_config_msaa},

    {52, "mov32",   TOKEN_ENCODE_MOV32,   TOKEN_ARITY_3,  (bool *)op_encode_mov32},
    {52, "t2_vmov", TOKEN_ENCODE_T2_VMOV_F32,  TOKEN_ARITY_2,  (bool *)op_encode_t2_vmov_f32},
    {52, "t1_movt", TOKEN_ENCODE_T1_MOVT, TOKEN_ARITY_2,  (bool *)op_encode_t1_movt},
    {52, "t1_mov",  TOKEN_ENCODE_T1_MOV,  TOKEN_ARITY_2,  (bool *)op_encode_t1_mov},
    {52, "t2_mov",  TOKEN_ENCODE_T2_MOV,  TOKEN_ARITY_3,  (bool *)op_encode_t2_mov},
    {52, "t3_mov",  TOKEN_ENCODE_T3_MOV,  TOKEN_ARITY_2,  (bool *)op_encode_t3_mov},
    {52, "a1_mov",  TOKEN_ENCODE_A1_MOV,  TOKEN_ARITY_3,  (bool *)op_encode_a1_mov},
    {52, "a2_mov",  TOKEN_ENCODE_A2_MOV,  TOKEN_ARITY_2,  (bool *)op_encode_a2_mov},
    {52, "bkpt",    TOKEN_ENCODE_BKPT,    TOKEN_CONSTANT, (bool *)op_encode_bkpt},
    {52, "nop",     TOKEN_ENCODE_NOP,     TOKEN_CONSTANT, (bool *)op_encode_nop},
    {52, "??",      TOKEN_ENCODE_UNK,     TOKEN_ARITY_1,  (bool *)op_encode_unk},

#ifdef BUILD_LEGACY_SUPPORT
    {0,  "<",       TOKEN_LEGACY,         0,              NULL},
#endif
    {0,  "$",       TOKEN_TERMINATOR,     0,              NULL},
    {0,  NULL,      TOKEN_INVALID,        0,              NULL}
};

void value_raw(value_t *lhs, int size) {
    lhs->type = DATA_TYPE_RAW;
    lhs->size = size;
    memset(&lhs->data.raw[size], 0, MAX_VALUE_SIZE - size);
}

/**
 * @brief Cast current value to another data type
 *         raw data can have arbitrary size
 *         primitives are always padded to 32bit
 *
 * @param lhs              value to cast
 * @param data_type        target data type
 */
void value_cast(value_t *lhs, value_data_type_t data_type) {
    if (lhs->type == data_type)
        goto ALIGN_SIZE; // nothing to do

    // * -> bytes
    if (data_type == DATA_TYPE_RAW) {
        lhs->type = DATA_TYPE_RAW;
        goto ALIGN_SIZE;
    }

    // bytes -> *
    if (lhs->type == DATA_TYPE_RAW) {
        lhs->type = data_type;
        memset(lhs->unk, 0, MAX_VALUE_SIZE * sizeof(bool));
        goto ALIGN_SIZE;
    }

    // * -> *
    switch (lhs->type) {
        case DATA_TYPE_SIGNED:
            switch (data_type) {
                default: break; // stfu
                case DATA_TYPE_UNSIGNED: // signed -> unsigned
                    lhs->data.uint32 = (uint32_t)lhs->data.int32;
                    break;
                case DATA_TYPE_FLOAT:    // signed -> float
                    lhs->data.fl32 = (float)lhs->data.int32;
                    break;
            }
            break;
        case DATA_TYPE_UNSIGNED:
            switch (data_type) {
                default: break; // stfu
                case DATA_TYPE_SIGNED:   // unsigned -> signed
                    lhs->data.int32 = (int32_t)lhs->data.uint32;
                    break;
                case DATA_TYPE_FLOAT:    // unsigned -> float
                    lhs->data.fl32 = (float)lhs->data.uint32;
                    break;
            }
            break;
         case DATA_TYPE_FLOAT:
            switch (data_type) {
                default: break; // stfu
                case DATA_TYPE_SIGNED:   // float -> signed
                    lhs->data.int32 = (int32_t)lhs->data.fl32;
                    break;
                case DATA_TYPE_UNSIGNED: // float -> unsigned
                    lhs->data.uint32 = (uint32_t)lhs->data.fl32;
                    break;
            }
            break;
        default: break; // stfu
    }

    lhs->type = data_type;

ALIGN_SIZE:
    if (lhs->type != DATA_TYPE_RAW)
        lhs->size = 4;
}

/**
 * @brief Cast LHS/RHS to common data type (if necessary)
 *         if LHS or RHS type is FLOAT - cast both to FLOAT
 *         if LHS or RHS type is SIGNED - cast both to SIGNED
 *
 * @param lhs              left-hand side
 * @param rhs              right-hand side
 */
void common_cast(value_t *lhs, value_t *rhs) {
    // Float
    if (lhs->type == DATA_TYPE_FLOAT || rhs->type == DATA_TYPE_FLOAT) {
        value_cast(lhs, DATA_TYPE_FLOAT);
        value_cast(rhs, DATA_TYPE_FLOAT);
    }
    // Signed
    else if (lhs->type == DATA_TYPE_SIGNED || rhs->type == DATA_TYPE_SIGNED) {
        value_cast(lhs, DATA_TYPE_SIGNED);
        value_cast(rhs, DATA_TYPE_SIGNED);
    }
}

/**
 * @brief Skip all whitespaces, increment pos
 *
 * @param expr             given expression
 * @param pos              starting position in given expression (offset)
 * @return                 false if EOF, true otherwise
 */
bool skip_ws(const char *expr, uint32_t *pos) {
    while (expr[*pos] != '\0' && isspace(expr[*pos])) (*pos)++;
    return expr[*pos] != '\0';
}

/**
 * @brief Parse raw hex bytes from the expression
 *
 * @param expr             given expression
 * @param pos              position in given expression (offset)
 * @param value            OUT - value
 * @param token            OUT - set to TOKEN_PRIMITIVE
 *
 * @return status
 */
intp_status_t parse_token_raw(const char *expr, uint32_t *pos, value_t *value, const token_t **token) {
    char *endptr = NULL;

    if (value != NULL) {
        value->type = DATA_TYPE_RAW;
        value->size = 0;
    }

    //printf("DEBUG: Parsing raw data at %d\n", *pos);

    while (isxdigit(expr[*pos])) {
        char byte[3];
        memcpy(&byte, &expr[*pos], 2);

        unsigned long tmp = strtoul(byte, &endptr, 16);
        if (endptr == byte) {
            //printf("Error: invalid format!\n");
            __intp_ret_status(INTP_STATUS_ERROR_INVALID_TOKEN, *pos);
        }

        (*pos) += endptr - byte;
        if (value != NULL) {
            value->data.raw[value->size] = tmp & 0xFF;
            value->size++;
        }

        // skip optional space between bytes
        if (!skip_ws(expr, pos))
            break; // nothing left to read?
    }

    if (expr[*pos] == 'r' || expr[*pos] == 'R') (*pos)++; // skip r char

    (*token) = TOKEN(TOKEN_PRIMITIVE);

    //printf("DEBUG: Raw data parsed %d\n", *pos);
    __intp_ret_status(INTP_STATUS_OK, *pos);
}

/**
 * @brief Parse decimal (aka. float) from the expression
 *
 * @param expr             given expression
 * @param pos              position in given expression (offset)
 * @param value            OUT - value
 * @param token            OUT - set to TOKEN_PRIMITIVE if token is decimal
 *
 * @return status
 */
intp_status_t parse_token_decimal(const char *expr, uint32_t *pos, value_t *value, const token_t **token) {
    char *endptr = NULL;
    float tmp = strtof(&expr[*pos], &endptr);

    // Found valid float
    if (endptr != &expr[*pos]) {
        (*pos) += endptr - &expr[*pos];
        (*token) = TOKEN(TOKEN_PRIMITIVE);
        if (value != NULL) {
            value->type = DATA_TYPE_FLOAT;
            value->data.fl32 = tmp;
            value->size = 4;
        }

        // Skip f char
        while (isspace(expr[*pos])) { (*pos)++; }
        if (expr[*pos] == 'f' || expr[*pos] == 'F') (*pos)++;
        __intp_ret_status(INTP_STATUS_OK, *pos);
    }

    __intp_ret_status(INTP_STATUS_ERROR_INVALID_TOKEN, *pos);
}

/**
 * @brief Parse integer from the expression
 *
 * @param expr             given expression
 * @param pos              position in given expression (offset)
 * @param value            OUT - value
 * @param token            OUT - set to TOKEN_PRIMITIVE if token is integer
 *
 * @return status
 */
intp_status_t parse_token_integer(const char *expr, uint32_t *pos, value_t *value, const token_t **token) {
    char *endptr = NULL;
    long tmp_l;
    unsigned long tmp_ul;
    bool is_signed = expr[*pos] == '-' || expr[*pos] == '+';

    if (is_signed)
        tmp_l = strtol(&expr[*pos], &endptr, 0);
    else
        tmp_ul = strtoul(&expr[*pos], &endptr, 0);

    // Found valid integer
    if (endptr != &expr[*pos]) {
        (*pos) += endptr - &expr[*pos];
        (*token) = TOKEN(TOKEN_PRIMITIVE);
        if (value != NULL) {
            if (is_signed) {
                value->type = DATA_TYPE_SIGNED;
                value->data.int32 = tmp_l;
            } else {
                value->type = DATA_TYPE_UNSIGNED;
                value->data.uint32 = tmp_ul;
            }
            value->size = 4;
        }
        __intp_ret_status(INTP_STATUS_OK, *pos);
    }

    __intp_ret_status(INTP_STATUS_ERROR_INVALID_TOKEN, *pos);
}

/**
 * @brief Parse single token from the expression
 *         (e.g. term, operator, function name, bracket...)
 *
 * @param expr             given expression
 * @param pos              starting position in given expression (offset)
 * @param value            OUT - parsed immediate value (if any)
 *                          can be set to NULL to ignore
 * @param token            OUT - parsed token
 * @param allow_immediate  parse immediate values
 *                          this is needed to differentiante between:
 *                              '+1' as in + == infix operator, 1 == RHS
 *                              '+1' as in +1 == immediate signed integer
 *                          if set to true, '+1' is treated as immediate value
 * @param force_raw        force raw hex byte input (if token is immediate value)
 *
 * @return status
 */
intp_status_t parse_token(const char *expr, uint32_t *pos, value_t *value, const token_t **token, bool allow_immediate, bool force_raw) {
    (*token) = TOKEN(TOKEN_INVALID);

    if (!skip_ws(expr, pos)) // Invalid token
        __intp_ret_status(INTP_STATUS_OK, *pos);

    //printf("DEBUG: parse_token(): raw?: %d\n", force_raw);

    // Immediate value
    if (allow_immediate && (isxdigit(expr[*pos]) || expr[*pos] == '+' || expr[*pos] == '-')) {
        uint32_t pos_peek = 0;

        bool is_numeric = false; // integer or decimal
        bool is_decimal = false; // is_numeric and decimal
        bool is_raw = false;

        // If not forcing raw input, check input type
        if (!force_raw) {
            // Peek digit after optional +/-
            pos_peek = *pos;
            //printf("DEBUG: pos_peek: %d '%c'\n", pos_peek, expr[pos_peek]);

            if (expr[pos_peek] == '-' || expr[pos_peek] == '+') { pos_peek++; }
            is_numeric = isdigit(expr[pos_peek]);

            // Peek . or f character (check if decimal)
            while (isdigit(expr[pos_peek])) { pos_peek++; }
            is_decimal = is_numeric && expr[pos_peek] == '.';
            if (!is_decimal) {
                while (isspace(expr[pos_peek])) { pos_peek++; }
                is_decimal = is_numeric && (expr[pos_peek] == 'f' || expr[pos_peek] == 'F');
            }

            // Peek r character (check if raw)
            pos_peek = *pos;
            while (isxdigit(expr[pos_peek]) || isspace(expr[pos_peek])) { pos_peek++; }
            is_raw = expr[pos_peek] == 'r' || expr[pos_peek] == 'R';

            //printf("DEBUG: numeric: %d, decimal: %d, raw: %d, force: %d\n", is_numeric, is_decimal, is_raw, force_raw);
        }

        // Raw data
        if (is_raw || force_raw) {
            return parse_token_raw(expr, pos, value, token);
        }
        // Decimal number
        else if (is_decimal) {
            return parse_token_decimal(expr, pos, value, token);
        }
        // Integer number
        else if (is_numeric) {
            return parse_token_integer(expr, pos, value, token);
        }
    }

    // Tokens
    for (int i = 0; i < TOKEN_INVALID; i++) {
        if (_TOKENS[i].string == NULL)
            continue;

        size_t token_len = strlen(_TOKENS[i].string);

        if (!strncasecmp(&expr[*pos], _TOKENS[i].string, token_len)) {
            //printf("DEBUG: Found token '%c' %d after-pos:%d\n", expr[*pos], i, *pos);

            (*pos) += token_len;
            (*token) = TOKEN(i);

            __intp_ret_status(INTP_STATUS_OK, *pos);
            //printf("DEBUG: After ret!\n");
        }
    }

    __intp_ret_status(INTP_STATUS_ERROR_INVALID_TOKEN, *pos);
}

/**
 * @brief Same as parse_token(), just doesn't increment pos counter
 */
intp_status_t peek_token(const char *expr, uint32_t pos, value_t *value, const token_t **token, bool allow_immediate, bool force_raw) {
    return parse_token(expr, &pos, value, token, allow_immediate, force_raw);
}

/**
 * @brief Parse and evaluate single function call or constant
 *
 * @param expr             given expression
 * @param pos              starting position in given expression (offset)
 * @param value            OUT - value
 * @param allow_brckt_next allow closing bracket ')' after constant, e.g. 'pi)'
 *
 * @return status
 */
intp_status_t parse_call(const char *expr, uint32_t *pos, value_t *value, bool allow_brckt_next) {
    const token_t *token;
    intp_status_t ret;
    uint32_t pos_error = 0;
    uint32_t pos_fn_begin = *pos;

    // Parse the function name
    ret = parse_token(expr, pos, value, &token, false, false);
    if (ret.code != INTP_STATUS_OK)
        return ret;

    // Constant?
    if ((token->flags & TOKEN_CONSTANT) && token->op != NULL) {
        // Apply op
        bool op_ret = ((bool (*)(value_t *))(token->op))(value);
        if (!op_ret)
            __intp_ret_status(INTP_STATUS_ERROR_INVALID_DATATYPE, pos_fn_begin);

        // Peek next token
        ret = peek_token(expr, *pos, NULL, &token, false, false);
        if (ret.code != INTP_STATUS_OK)
            return ret;

#ifdef BUILD_LEGACY_SUPPORT
        // Ignore optional () braces used with constants

        // Is next token opening bracket?
        if (token->type == TOKEN_BRACKET_OPEN) {
            // Skip it
            parse_token(expr, pos, NULL, &token, false, false);

            // Match closing bracket!
            pos_error = *pos;
            ret = parse_token(expr, pos, NULL, &token, true, false);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            if (token->type != TOKEN_BRACKET_CLOSE)
                __intp_ret_status(INTP_STATUS_ERROR_TOO_MANY_ARGS, pos_error);
        }
#else
        // Don't allow 'pi)'
        if (token->type != TOKEN_INVALID
                && token->type != TOKEN_TERMINATOR
                && !(token->flags & TOKEN_INFIX)
                && (!allow_brckt_next || token->type != TOKEN_BRACKET_CLOSE))
            __intp_ret_status(INTP_STATUS_ERROR_INVALID_TOKEN, *pos);
#endif
    }

    // Function call
    else if (((token->flags & TOKEN_ARITY_1)
            || (token->flags & TOKEN_ARITY_2)
            || (token->flags & TOKEN_ARITY_3)
            || (token->flags & TOKEN_ARITY_4)) && token->op != NULL) {
        const token_t *op = token;
        value_t arg[TOKEN_ARITY_4 - 1];
        memset(arg, 0, (TOKEN_ARITY_4 - 1) * sizeof(value_t));

        // Check for opening bracket
        pos_error = *pos;
        ret = parse_token(expr, pos, NULL, &token, false, false);
        if (ret.code != INTP_STATUS_OK)
            return ret;

        // Missing? Fek
        if (token->type != TOKEN_BRACKET_OPEN)
            __intp_ret_status(INTP_STATUS_ERROR_MISSING_OPEN_BRACKET, pos_error);

        // Peek and set proper error code if next token is closing bracket,
        // and not an actual argument
        ret = peek_token(expr, *pos, NULL, &token, true, (op->flags & TOKEN_FORCE_RAW));
        if (ret.code != INTP_STATUS_OK)
            return ret;
        if (token->type == TOKEN_BRACKET_CLOSE)
            __intp_ret_status(INTP_STATUS_ERROR_TOO_FEW_ARGS, *pos);

        int argn = TOKEN_GET_ARGN(op);

        // Grab and evaluate first argument
        if (op->flags & TOKEN_FORCE_RAW)
            ret = parse_value(expr, pos, value, true, true);
        else
            ret = parse_expression(expr, pos, value, false, true);
        if (ret.code != INTP_STATUS_OK)
            return ret;

        // Grab and evaluate additional arguments (if necessary)
        for (int i = 0; i < argn - 1; i++) {
            // Check for arg. separator token
            pos_error = *pos;
            ret = parse_token(expr, pos, NULL, &token, false, false);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            if (token->type != TOKEN_ARGUMENT_SEP)
                __intp_ret_status(INTP_STATUS_ERROR_TOO_FEW_ARGS, pos_error);

            // Grab and evaluate (i+1)-th argument
            if (op->flags & TOKEN_FORCE_RAW)
                ret = parse_value(expr, pos, &arg[i], true, true);
            else
                ret = parse_expression(expr, pos, &arg[i], false, true);

            if (ret.code != INTP_STATUS_OK)
                return ret;
        }

        // Match closing bracket!
        pos_error = *pos;
        ret = parse_token(expr, pos, NULL, &token, false, false);
        if (ret.code != INTP_STATUS_OK)
            return ret;

        if (token->type == TOKEN_ARGUMENT_SEP)
            __intp_ret_status(INTP_STATUS_ERROR_TOO_MANY_ARGS, pos_error);
        if (token->type != TOKEN_BRACKET_CLOSE)
            __intp_ret_status(INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET, pos_error);

        // Apply op
        bool op_ret = false;
        switch (TOKEN_GET_ARGN(op)) {
            case 1: op_ret = ((bool (*)(value_t *))(op->op))(value); break;
            case 2: op_ret = ((bool (*)(value_t *, value_t *))(op->op))(value, &arg[0]); break;
            case 3: op_ret = ((bool (*)(value_t *, value_t *, value_t *))(op->op))(value, &arg[0], &arg[1]); break;
            case 4: op_ret = ((bool (*)(value_t *, value_t *, value_t *, value_t *))(op->op))(value, &arg[0], &arg[1], &arg[2]); break;
            default: /*printf("DEBUG: Invalid fn argn! %d\n", TOKEN_GET_ARGN(op));*/ break;
        }
        if (!op_ret)
            __intp_ret_status(INTP_STATUS_ERROR_INVALID_DATATYPE, pos_fn_begin);
    }

    __intp_ret_status(INTP_STATUS_OK, *pos);
}

/**
 * @brief Parse and evalute LHS/RHS
 *         if next token is immediate:
 *             return the value
 *
 *         if next token is opening bracket
 *             evaluate inner expression using parse_expression()
 *             match closing bracket
 *             return the value
 *
 *         if next token is function call:
 *             evaluate the function call using parse_call()
 *             return the value
 *
 *         if next token is TOKEN_LEGACY:
 *             evaluate legacy macro
 *             return the value
 *
 * @param expr             given expression
 * @param pos              position in given expression (offset)
 * @param value            OUT - value
 * @param force_raw        force raw hex byte input (if token is immediate value)
 * @param allow_brckt_next allow closing bracket ')' after parsed value
 * @return
 */
intp_status_t parse_value(const char *expr, uint32_t *pos, value_t *value, bool force_raw, bool allow_brckt_next) {
    const token_t *token;
    intp_status_t ret;
    uint32_t pos_error;
    memset(value, 0, sizeof(value_t));

    // Peek first, check if token is function call
    ret = peek_token(expr, *pos, value, &token, true, force_raw);
    if (ret.code != INTP_STATUS_OK)
        return ret;

    // Value has to be a value and cannot start with infix operator or ')'
    if (token->flags & TOKEN_INFIX
            || token->type == TOKEN_BRACKET_CLOSE
            || token->type == TOKEN_INVALID
            || token->type == TOKEN_TERMINATOR) {
        skip_ws(expr, pos); // incr. pos to token
        __intp_ret_status(INTP_STATUS_ERROR_INVALID_TOKEN, *pos);
    }

    //printf("- parse_value(): val: 0x%x, data_type: %d, token_type: %d\n", value->data.uint32, value->type, token->type);

#ifdef BUILD_LEGACY_SUPPORT
    // Legacy backwards compat.
    if (token->type == TOKEN_LEGACY) {
        ret = legacy_parse_gen(expr, pos, value);
        return ret;
    }
#endif

    // Function call? Evaluate now!
    if ((token->flags & TOKEN_CONSTANT) ||
            (token->flags & TOKEN_ARITY_1) ||
            (token->flags & TOKEN_ARITY_2) ||
            (token->flags & TOKEN_ARITY_3) ||
            (token->flags & TOKEN_ARITY_4)) {
        return parse_call(expr, pos, value, allow_brckt_next);
    }

    // Skip the peeked token
    parse_token(expr, pos, value, &token, true, force_raw);

    // Brackets?
    if (token->type == TOKEN_BRACKET_OPEN) {
        // Evaluate inside
        ret = parse_expression(expr, pos, value, force_raw, true);
        if (ret.code != INTP_STATUS_OK) return ret;

        // Match closing bracket!
        pos_error = *pos;
        ret = parse_token(expr, pos, NULL, &token, true, false);
        if (ret.code != INTP_STATUS_OK) return ret;
        if (token->type != TOKEN_BRACKET_CLOSE)
            __intp_ret_status(INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET, pos_error);
    }

    else {
        // Peek next
        ret = peek_token(expr, *pos, NULL, &token, false, false);
        if (ret.code != INTP_STATUS_OK)
            return ret;

        // Don't allow '1)'
        if (token->type != TOKEN_INVALID
                    && !(token->flags & TOKEN_INFIX)
                    && token->type != TOKEN_ARGUMENT_SEP
                    && token->type != TOKEN_TERMINATOR
                    && (!allow_brckt_next || token->type != TOKEN_BRACKET_CLOSE)) {
                __intp_ret_status(INTP_STATUS_ERROR_INVALID_TOKEN, *pos);
        }
    }

    // All ok :)
    __intp_ret_status(INTP_STATUS_OK, *pos);
}

/**
 * @brief Parse and evaluate binary expression subtree
 *         aka. precedence climbing algorithm
 *
 *         value has to be already evaluated LHS,
 *         &expr[pos] should start with next operator (if any)
 *
 * @param expr             given expression
 * @param pos              position in given expression (offset)
 *                          starting with infix op + RHS
 * @param value            OUT - value
 * @param min_precedence   minimal precedence
 *
 * @return status
 */
intp_status_t parse_subtree(const char *expr, uint32_t *pos, value_t *value, int min_precedence) {
    if (!skip_ws(expr, pos))
        __intp_ret_status(INTP_STATUS_OK, *pos);

    uint32_t pos_infix_op = *pos;
    intp_status_t ret;
    value_t rhs = {0};
    const token_t *token;

    // Peek next token (operator)
    ret = peek_token(expr, *pos, NULL, &token, false, false);
    if (ret.code != INTP_STATUS_OK)
        return ret;

    // Handle terminating token
    if (token->type == TOKEN_TERMINATOR)
        __intp_ret_status(INTP_STATUS_OK, *pos);

    // While next token is infix operator with minimal precedence
    while ((token->flags & TOKEN_INFIX)
            && token->precedence >= min_precedence
            && token->op != NULL) {
        const token_t *op = token;

        // Advance to next token (skip infix)
        ret = parse_token(expr, pos, NULL, &token, false, false);
        if (ret.code != INTP_STATUS_OK)
            return ret;

        // Parse RHS
        ret = parse_value(expr, pos, &rhs, false, true);
        if (ret.code != INTP_STATUS_OK)
            return ret;

        // Peek next token
        ret = peek_token(expr, *pos, NULL, &token, false, false);
        if (ret.code != INTP_STATUS_OK)
            return ret;

        // If next infix token has higher precedence, evaluate RHS first!
        while ((token->flags & TOKEN_INFIX)
                && token->precedence > op->precedence) {
            // Honor precedence
            ret = parse_subtree(expr, pos, &rhs, token->precedence);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            // Peek next token
            ret = peek_token(expr, *pos, NULL, &token, false, false);
            if (ret.code != INTP_STATUS_OK)
                return ret;
        }

        // Apply op
        bool op_ret = ((bool (*)(value_t *, value_t *))(op->op))(value, &rhs);
        if (!op_ret)
            __intp_ret_status(INTP_STATUS_ERROR_INVALID_DATATYPE, pos_infix_op);
    }

    __intp_ret_status(INTP_STATUS_OK, *pos);
}

/**
 * @brief Parse and evaluate expression
 *         e.g. '3 * (4 - 2) + 14'
 *
 * @param expr             given expression
 * @param pos              position in given expression (offset)
 * @param value            OUT - value
 * @param force_raw        force raw hex byte input
 * @param allow_brckt_next allow bracket after 1st lhs, e.g. '1)'
 *
 * @return status
 */
intp_status_t parse_expression(const char *expr, uint32_t *pos, value_t *value, bool force_raw, bool allow_brckt_next) {
    intp_status_t ret;

    //printf("DEBUG: parse_expression(): Raw?: %d\n", force_raw);

    // Parse LHS
    ret = parse_value(expr, pos, value, force_raw, allow_brckt_next);
    if (ret.code != INTP_STATUS_OK)
        return ret;

    // Go!
    ret = parse_subtree(expr, pos, value, 0);
    return ret;
}
