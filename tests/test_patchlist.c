#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/interpreter/interpreter.h"

#define LINE_SIZE 4096

static const char *skip_ws(const char *text) {
    while (isspace((unsigned char)*text)) {
        text++;
    }
    return text;
}

static bool parse_patch_line(const char *line, uint8_t *segment, uint32_t *offset, const char **expression) {
    char *end;
    errno = 0;
    unsigned long parsed_segment = strtoul(line, &end, 10);
    if (errno || end == line || parsed_segment > UINT8_MAX || *end != ':') {
        return false;
    }

    line = end + 1;
    errno = 0;
    unsigned long parsed_offset = strtoul(line, &end, 0);
    if (errno || end == line || parsed_offset > UINT32_MAX || !isspace((unsigned char)*end)) {
        return false;
    }

    *expression = skip_ws(end);
    if (**expression == '\0') {
        return false;
    }

    *segment = parsed_segment;
    *offset = parsed_offset;
    return true;
}

static bool parse_resolution(const char *text, const char **end, uint16_t *width, uint16_t *height) {
    char *next;
    errno = 0;
    unsigned long parsed_width = strtoul(text, &next, 10);
    if (errno || next == text || parsed_width == 0 || parsed_width > UINT16_MAX || (*next != 'x' && *next != 'X')) {
        return false;
    }

    text = next + 1;
    errno = 0;
    unsigned long parsed_height = strtoul(text, &next, 10);
    if (errno || next == text || parsed_height == 0 || parsed_height > UINT16_MAX) {
        return false;
    }

    *width = parsed_width;
    *height = parsed_height;
    *end = next;
    return true;
}

static bool parse_ib_list(const char *text, intp_vg_context_t *context) {
    uint8_t count = 0;
    while (*text != '\0') {
        if (count >= INTP_VG_MAX_RES_COUNT) {
            return false;
        }

        const char *end;
        if (!parse_resolution(text, &end, &context->ib_width[count], &context->ib_height[count])) {
            return false;
        }
        count++;
        if (*end == '\0') {
            break;
        }
        if (*end != ',') {
            return false;
        }
        text = end + 1;
    }

    if (count == 0) {
        return false;
    }
    for (; count < INTP_VG_MAX_RES_COUNT; count++) {
        context->ib_width[count] = context->ib_width[count - 1];
        context->ib_height[count] = context->ib_height[count - 1];
    }
    return true;
}

static bool parse_fps(const char *text, intp_vg_context_t *context) {
    if (!strcmp(text, "60")) {
        context->vblank = 1;
        context->fps_limit = 60;
    } else if (!strcmp(text, "30")) {
        context->vblank = 2;
        context->fps_limit = 30;
    } else if (!strcmp(text, "20")) {
        context->vblank = 3;
        context->fps_limit = 20;
    } else {
        return false;
    }
    return true;
}

static bool parse_msaa(const char *text, intp_vg_context_t *context) {
    if (!strcmp(text, "0") || !strcmp(text, "1") || !strcmp(text, "2")) {
        context->msaa = text[0] - '0';
        context->msaa_enabled = context->msaa > 0;
        return true;
    }
    return false;
}

static void write_result(unsigned int line_number, uint8_t segment,
        uint32_t offset, intp_status_t status, const intp_value_t *value) {
    fprintf(stdout, "%05u %u:%08X ", line_number, segment, offset);
    if (status.code != INTP_STATUS_OK) {
        fprintf(stdout, "ERR %d %u\n", status.code, status.pos);
        return;
    }

    fprintf(stdout, "OK %d %u", value->type, value->size);
    for (uint8_t i = 0; i < value->size; i++) {
        fprintf(stdout, " %02X", value->data.raw[i]);
    }
    fputc('\n', stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <patchlist.txt> [--fb WIDTHxHEIGHT] [--ib WIDTHxHEIGHT[,WIDTHxHEIGHT...]] [--fps 20|30|60] [--msaa 0|1|2]\n", argv[0]);
        return 2;
    }

    intp_vg_context_t context = {0};
    context.fb_width = 960;
    context.fb_height = 544;
    for (int i = 0; i < INTP_VG_MAX_RES_COUNT; i++) {
        context.ib_width[i] = 960;
        context.ib_height[i] = 544;
    }
    context.vblank = 1;
    context.fps_limit = 60;
    context.msaa = 2;
    context.msaa_enabled = true;

    for (int i = 2; i < argc; i += 2) {
        if (i + 1 >= argc) {
            fprintf(stderr, "Missing value for %s.\n", argv[i]);
            return 2;
        }

        if (!strcmp(argv[i], "--fb")) {
            const char *end;
            if (!parse_resolution(argv[i + 1], &end, &context.fb_width, &context.fb_height)
                    || *end != '\0') {
                fprintf(stderr, "Invalid framebuffer resolution: %s\n", argv[i + 1]);
                return 2;
            }
        } else if (!strcmp(argv[i], "--ib")) {
            if (!parse_ib_list(argv[i + 1], &context)) {
                fprintf(stderr, "Invalid internal-buffer resolution list: %s\n", argv[i + 1]);
                return 2;
            }
        } else if (!strcmp(argv[i], "--fps")) {
            if (!parse_fps(argv[i + 1], &context)) {
                fprintf(stderr, "Invalid frame rate: %s\n", argv[i + 1]);
                return 2;
            }
        } else if (!strcmp(argv[i], "--msaa")) {
            if (!parse_msaa(argv[i + 1], &context)) {
                fprintf(stderr, "Invalid MSAA value: %s\n", argv[i + 1]);
                return 2;
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 2;
        }
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL) {
        perror(argv[1]);
        return 2;
    }

    intp_set_vg_context(&context);

    char line[LINE_SIZE];
    unsigned int line_number = 0;
    unsigned int patch_count = 0;
    unsigned int error_count = 0;
    while (fgets(line, sizeof(line), input) != NULL) {
        line_number++;

        size_t length = strlen(line);
        if (length == sizeof(line) - 1 && line[length - 1] != '\n') {
            fprintf(stderr, "%s:%u: line is too long\n", argv[1], line_number);
            error_count++;
            int character;
            while ((character = fgetc(input)) != '\n' && character != EOF) {}
            continue;
        }

        line[strcspn(line, "\r\n#")] = '\0';
        const char *patch_line = skip_ws(line);
        uint8_t segment;
        uint32_t offset;
        const char *expression;
        if (!parse_patch_line(patch_line, &segment, &offset, &expression)) {
            continue;
        }

        intp_value_t value = {0};
        uint32_t position = 0;
        intp_status_t status = intp_evaluate(expression, &position, &value);
        write_result(line_number, segment, offset, status, &value);
        patch_count++;
        if (status.code != INTP_STATUS_OK) {
            error_count++;
        }
    }

    if (ferror(input)) {
        perror(argv[1]);
        error_count++;
    }
    fclose(input);

    fprintf(stderr, "%u patch expressions written (%u errors)\n", patch_count, error_count);
    return error_count == 0 ? 0 : 1;
}
