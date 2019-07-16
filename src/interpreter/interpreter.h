#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <stdint.h>

typedef uint8_t byte_t;

#ifdef BUILD_VITAGRAFIX
int isdigit(int c);
int isxdigit(int c);
int isspace(int c);
int tolower(int c);
#endif

#define MAX_VALUE_SIZE 32

typedef enum {
    DATA_TYPE_SIGNED,
    DATA_TYPE_UNSIGNED,
    DATA_TYPE_FLOAT,
    DATA_TYPE_RAW
} intp_value_data_type_t;

typedef union {
    // primitive representation (32b)
    int32_t int32;
    uint32_t uint32;
    float fl32;

    // raw representation (up to MAX_VALUE_SIZE)
    byte_t raw[MAX_VALUE_SIZE];
} intp_value_data_t;

typedef struct {
    intp_value_data_t data;
    intp_value_data_type_t type;

    // data size in bytes (primitives = 4B)
    byte_t size;
} intp_value_t;

typedef enum {
    INTP_STATUS_OK,
    // Generic
    INTP_STATUS_ERROR_SYNTAX,
    INTP_STATUS_ERROR_IO,

    INTP_STATUS_ERROR_UNEXPECTED_EOF,

    INTP_STATUS_ERROR_INVALID_DATATYPE,
    INTP_STATUS_ERROR_INVALID_TOKEN,
    INTP_STATUS_ERROR_TOO_FEW_ARGS,
    INTP_STATUS_ERROR_TOO_MANY_ARGS,
    INTP_STATUS_ERROR_MISSING_OPEN_BRACKET,
    INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET,

    INTP_STATUS_ERROR_VG_IB_OOB,

    INTP_STATUS_MAX
} intp_status_code_t;

typedef struct {
    intp_status_code_t code;
    uint32_t pos;
} intp_status_t;

intp_status_t intp_evaluate(const char *expression, uint32_t *pos, intp_value_t *value);
void intp_format_error(const char *expression, intp_status_t status, char *destination, uint32_t size);
const char *intp_data_type_to_string(intp_value_data_type_t type);
const char *intp_status_code_to_string(intp_status_code_t code);

#endif
