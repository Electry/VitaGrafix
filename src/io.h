#ifndef _IO_H_
#define _IO_H_
#include <stdbool.h>

#define IO_MAX_LINE_SIZE 128
#define IO_CHUNK_SIZE    1024

#define __ret_status(code, line, pos_line) {\
    vg_io_status_t ret = {code, line, pos_line};\
    return ret;\
}

typedef enum {
    IO_OK,

    // I/O error
    IO_ERROR_LINE_TOO_LONG,
    IO_ERROR_OPEN_FAILED,

    // Syntax error
    IO_ERROR_PARSE_INVALID_TOKEN,
    IO_ERROR_INTERPRETER_ERROR
} vg_io_status_code_t;

typedef struct {
    vg_io_status_code_t code;
    uint32_t line;
    uint32_t pos_line;
} vg_io_status_t;

const char *vg_io_status_code_to_string(vg_io_status_code_t code);
vg_io_status_t vg_io_parse(const char *path, vg_io_status_t (*parse_line_fn)(const char line[]), bool create);

#endif
