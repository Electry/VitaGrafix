#include <vitasdk.h>
#include <taihen.h>
#include <stdlib.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "main.h"

int vg_io_find_eol(const char chunk[], int pos, int end, bool allow_nonlf_eol) {
    for (int i = pos; i < end; i++) {
        if (chunk[i] == '\n'
                || (allow_nonlf_eol && (chunk[i] == '#' || chunk[i] == '\r'))) {
            return i;
        }
    }
    return -1;
}

const char *vg_io_status_code_to_string(vg_io_status_code_t code) {
    switch (code) {
        case IO_OK: return "All OK!";
        case IO_ERROR_LINE_TOO_LONG: return "Line too long.";
        case IO_ERROR_OPEN_FAILED: return "Failed to open the file.";
        case IO_ERROR_PARSE_INVALID_TOKEN: return "Invalid token.";
        case IO_ERROR_INTERPRETER_ERROR: return "Interpreter error.";
        case IO_ERROR_TOO_MANY_PATCHES: return "Too many patches, limit: " TOSTRING(MAX_INJECT_NUM);
        case IO_ERROR_TOO_MANY_HOOKS: return "Too many hooks, limit: " TOSTRING(MAX_HOOK_NUM);
        case IO_ERROR_TAI_PATCH_EXISTS: return "Memory already patched.";
        case IO_ERROR_TAI_GENERIC: return "Unknown TAI error.";
        default: return "?";
    }
}

vg_io_status_t vg_io_parse_section_header(const char line[], char titleid[], char self[], uint32_t *nid) {
    size_t len = strlen(line);
    int pos = 0;

    if (TITLEID_LEN + 1 >= len) // Line too short?
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, len);

    // Set defaults
    strncpy(titleid, TITLEID_ANY, TITLEID_LEN);
    self[0] = '\0'; // SELF_ANY
    *nid = NID_ANY;

    // Match opening bracket '['
    while (isspace(line[pos])) { pos++; }
    if (line[pos] != '[')
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
    pos++;

    // TITLEID (required)
    while (isspace(line[pos])) { pos++; }
    strncpy(titleid, &line[pos], TITLEID_LEN);
    pos += TITLEID_LEN;

    // SELF & NID (optional)
    while (isspace(line[pos])) { pos++; }
    if (line[pos] == ',') {
        pos++;

        // Peek next separator ',' or ']'
        int pos_sep = pos;
        while (line[pos_sep] != '\0'
                && line[pos_sep] != ','
                && line[pos_sep] != ']') { pos_sep++; }
        if (line[pos_sep] == '\0')
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos_sep);

        // SELF
        while (isspace(line[pos])) { pos++; }
        strncpy(self, &line[pos], pos_sep - pos);
        pos = pos_sep;

        // NID
        if (line[pos] == ',') {
            pos++;
            while (isspace(line[pos])) { pos++; }
            char *end = NULL;
            *nid = strtoul(&line[pos], &end, 0);
            if (end == &line[pos])
                __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

            pos += (end - &line[pos]);
        }
    }

    // Match closing bracket ']'
    while (isspace(line[pos])) { pos++; }
    if (line[pos] != ']')
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

    __ret_status(IO_OK, 0, pos);
}

vg_io_status_t vg_io_parse(const char *path, vg_io_status_t (*parse_line_fn)(const char line[]), bool create) {
    vg_io_status_t ret = {IO_OK, 0, 0};

    SceUID fd = sceIoOpen(path, SCE_O_RDONLY | (create ? SCE_O_CREAT : 0), 0777);
    if (fd < 0) {
        ret.code = IO_ERROR_OPEN_FAILED;
        goto EXIT;
    }

    int chunk_size = 0;
    char chunk[IO_CHUNK_SIZE] = "";
    int pos = 0, end = 0, end_readable = 0;
    int line_counter = 0;

    while ((chunk_size = sceIoRead(fd, chunk, IO_CHUNK_SIZE)) > 1) {
#ifdef ENABLE_VERBOSE_LOGGING
        vg_log_printf("[IO] DEBUG: Parsing new chunk, size=%d\n", chunk_size);
#endif
        pos = 0;

        // Read all lines in chunk
        while (pos < chunk_size) {
            // Get next real EOL
            end = vg_io_find_eol(chunk, pos, chunk_size, false);

            // Did not find EOL in current chunk & there is more to read?
            // Proceed by reading next sub-chunk
            if (end == -1 && chunk_size == IO_CHUNK_SIZE) {
#ifdef ENABLE_VERBOSE_LOGGING
                vg_log_printf("[IO] DEBUG: Didnt find EOL in this chunk, pos=%d, seek=%d\n",
                        pos, 0 - (IO_CHUNK_SIZE - pos));
#endif
                // Single line is > CONFIG_CHUNK_SIZE chars
                if (pos == 0) {
                    ret.code = IO_ERROR_LINE_TOO_LONG;
                    ret.line = line_counter;
                    goto EXIT_CLOSE;
                }

                // Seek back to where unfinished line started
                sceIoLseek(fd, 0 - (IO_CHUNK_SIZE - pos), SCE_SEEK_CUR);
                break;
            }
            // Found EOL, parse line
            else {
                line_counter++;

                // Last chunk, last line, no EOL?
                if (end == -1) {
#ifdef ENABLE_VERBOSE_LOGGING
                    vg_log_printf("[IO] WARN: Last line is missing EOL char!\n");
#endif
                    end = chunk_size;
                }

                // Ignore leading spaces and tabs
                while (pos < end && isspace(chunk[pos])) { pos++; }
                if (pos == end)
                    goto NEXT_LINE; // Empty line

                // Ignore comments
                if (chunk[pos] == '#')
                    goto NEXT_LINE;

                // Search for '#'
                end_readable = vg_io_find_eol(chunk, pos, end, true);
                if (end_readable == -1) // No '#' found on this line
                    end_readable = end; // Set eol to '\n'

                // Swap \n or # with \0
                chunk[end_readable] = '\0';

                // Parse valid line
                ret = parse_line_fn(&chunk[pos]);
                if (ret.code != IO_OK) {
                    ret.line = line_counter;
                    goto EXIT_CLOSE;
                }

                // Swap back \n
                chunk[end_readable] = '\n';

NEXT_LINE:
                pos = end + 1;
            }
        }

        // EOF
        if (chunk_size < IO_CHUNK_SIZE)
            break;
    }

EXIT_CLOSE:
    sceIoClose(fd);
EXIT:
    return ret;
}
