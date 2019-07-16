#include <stdio.h>
#include <string.h>

#include "interpreter.h"
#include "parser.h"

intp_status_t intp_evaluate(const char *expression, uint32_t *pos, intp_value_t *value) {
    return parse_expression(expression, pos, value, false, false);
}

void intp_format_error(const char *expression, intp_status_t status, char *destination, uint32_t size) {
    if (status.code == INTP_STATUS_OK) {
        snprintf(destination, size, "No error!\n");
        return;
    }

    snprintf(destination, size, "ERROR near:\n    %s\n    %*s^\n%s",
            expression, (int)status.pos, status.pos == 0 ? "" : " ", intp_status_code_to_string(status.code));
}

const char *intp_status_code_to_string(intp_status_code_t code) {
    switch (code) {
        case INTP_STATUS_OK:
            return "No error!";
        case INTP_STATUS_ERROR_SYNTAX:
            return "Syntax error.";
        case INTP_STATUS_ERROR_IO:
            return "I/O error.";
        case INTP_STATUS_ERROR_UNEXPECTED_EOF:
            return "Unexpected end of expression.";
        case INTP_STATUS_ERROR_INVALID_DATATYPE:
            return "Invalid argument data type supplied to the function/operator.";
        case INTP_STATUS_ERROR_INVALID_TOKEN:
            return "Invalid token.";
        case INTP_STATUS_ERROR_TOO_FEW_ARGS:
            return "Too few arguments supplied to the function.";
        case INTP_STATUS_ERROR_TOO_MANY_ARGS:
            return "Too many arguments supplied to the function.";
        case INTP_STATUS_ERROR_MISSING_OPEN_BRACKET:
            return "Missing opening bracket.";
        case INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET:
            return "Missing closing bracket.";
        case INTP_STATUS_ERROR_VG_IB_OOB:
            return "Internal buffer index out of bounds.";
        case INTP_STATUS_MAX:
        default:
            return "?";
    }
}

const char *intp_data_type_to_string(intp_value_data_type_t type) {
    switch (type) {
        case DATA_TYPE_SIGNED:   return "Signed integer";
        case DATA_TYPE_UNSIGNED: return "Unsigned integer";
        case DATA_TYPE_FLOAT:    return "Floating point";
        case DATA_TYPE_RAW:      return "Raw";
    }
    return "?";
}
