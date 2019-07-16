#include <string.h>
#ifndef BUILD_VITAGRAFIX
#include <ctype.h>
#endif
#include <stdlib.h>
#include <stdbool.h>

#ifdef BUILD_VITAGRAFIX
#include <vitasdk.h>
#include <taihen.h>
#include "../io.h"
#include "../config.h"
#include "../log.h"
#include "../main.h"
#endif

#include "interpreter.h"
#include "parser.h"


intp_status_t legacy_parse_gen_value(
        const char chunk[], int pos, int end,
        uint32_t *value);

//int vgPatchGetNextMacroArgPos()
int legacy_get_next_macro_arg_pos(const char chunk[], int pos, int end) {
    int inner_open = 0;

    while (pos < end - 3 && (inner_open > 0 || chunk[pos] != ',')) {
        // Allow stacking, e.g.: </,<*,<ib_w>,10>,10>
        if (chunk[pos] == '<')
            inner_open++;
        if (chunk[pos] == '>')
            inner_open--;
        pos++;
    }

    return pos + 1;
}

intp_status_t legacy_parse_gen(const char *expr, uint32_t *pos, value_t *value) {
    int end = *pos;
    while (expr[end] != '\0' && expr[end] != ')') { end++; }
    if (expr[end] == '\0')
        __intp_ret_status(INTP_STATUS_ERROR_UNEXPECTED_EOF, end);

    uint32_t legacy_value = 0;
    intp_status_t ret = legacy_parse_gen_value(expr, *pos, end, &legacy_value);
    if (ret.code != INTP_STATUS_OK)
        return ret;

    value->type = DATA_TYPE_UNSIGNED;
    value->data.uint32 = legacy_value;
    value->size = 4;
    //printf("Before pos: %d\n", *pos);
    (*pos) += (end - *pos);
    //printf("After pos: %d\n", *pos);

    return ret;
}


// VG_IoParseState vgPatchParseGenValue()
intp_status_t legacy_parse_gen_value(
        const char chunk[], int pos, int end,
        uint32_t *value) {
    intp_status_t ret = {INTP_STATUS_OK, 0};

    // Check for macros
    if (chunk[pos] == '<') {
#ifdef BUILD_VITAGRAFIX
        if (!strncasecmp(&chunk[pos], "<fb_w>", 6)) {
            *value = g_main.config.fb.width;
            return ret;
        }
        if (!strncasecmp(&chunk[pos], "<fb_h>", 6)) {
            *value = g_main.config.fb.height;
            return ret;
        }
        if (!strncasecmp(&chunk[pos], "<ib_w", 5)) {
            if (chunk[pos + 5] == '>') {
                *value = g_main.config.ib[0].width;
                return ret;
            }
            else if (chunk[pos + 5] == ',') {
                uint8_t ib_n = strtoul(&chunk[pos + 6], NULL, 10);
                if (ib_n >= MAX_RES_COUNT) {
                    vg_log_printf("[PATCH] ERROR: Accessed [%u] IB res out of range!\n", ib_n);
                    __intp_ret_status(INTP_STATUS_ERROR_VG_IB_OOB, pos + 6);
                }

                vg_config_set_supported_ib_count(ib_n + 1); // Raise supp. IB count
                *value = g_main.config.ib[ib_n].width;
                return ret;
            }
        }
        if (!strncasecmp(&chunk[pos], "<ib_h", 5)) {
            if (chunk[pos + 5] == '>') {
                *value = g_main.config.ib[0].height;
                return ret;
            }
            else if (chunk[pos + 5] == ',') {
                uint8_t ib_n = strtoul(&chunk[pos + 6], NULL, 10);
                if (ib_n >= MAX_RES_COUNT) {
                    vg_log_printf("[PATCH] ERROR: Accessed [%u] IB res out of range!\n", ib_n);
                    __intp_ret_status(INTP_STATUS_ERROR_VG_IB_OOB, pos + 6);
                }

                vg_config_set_supported_ib_count(ib_n + 1); // Raise supp. IB count
                *value = g_main.config.ib[ib_n].height;
                return ret;
            }
        }
        if (!strncasecmp(&chunk[pos], "<vblank>", 8)) {
            *value = g_main.config.fps == FPS_60 ? 1 : 2;
            return ret;
        }
        if (!strncasecmp(&chunk[pos], "<msaa>", 6)) {
            *value = g_main.config.msaa == MSAA_4X ? 2 :
                    (g_main.config.msaa == MSAA_2X ? 1 : 0);
            return ret;
        }
        if (!strncasecmp(&chunk[pos], "<msaa_enabled>", 14)) {
            *value = g_main.config.msaa_enabled == FT_ENABLED;
            return ret;
        }
#endif
        if (!strncasecmp(&chunk[pos], "<+,", 3) ||
                !strncasecmp(&chunk[pos], "<-,", 3) ||
                !strncasecmp(&chunk[pos], "<*,", 3) ||
                !strncasecmp(&chunk[pos], "</,", 3) ||
                !strncasecmp(&chunk[pos], "<&,", 3) ||
                !strncasecmp(&chunk[pos], "<|,", 3) ||
                !strncasecmp(&chunk[pos], "<l,", 3) ||
                !strncasecmp(&chunk[pos], "<r,", 3) ||
                !strncasecmp(&chunk[pos], "<min,", 5) ||
                !strncasecmp(&chunk[pos], "<max,", 5)) {
            int token_pos = pos + 3;
            if (tolower(chunk[pos + 1]) == 'm') {
                token_pos += 2;
            }
            uint32_t a, b;

            ret = legacy_parse_gen_value(chunk, token_pos, end, &a);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            token_pos = legacy_get_next_macro_arg_pos(chunk, token_pos, end);
            ret = legacy_parse_gen_value(chunk, token_pos, end, &b);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            if (chunk[pos + 1] == '+')
                *value = a + b;
            else if (chunk[pos + 1] == '-')
                *value = a - b;
            else if (chunk[pos + 1] == '*')
                *value = a * b;
            else if (chunk[pos + 1] == '/')
                *value = a / b;
            else if (chunk[pos + 1] == '&')
                *value = a & b;
            else if (chunk[pos + 1] == '|')
                *value = a | b;
            else if (tolower(chunk[pos + 1]) == 'l')
                *value = a << b;
            else if (tolower(chunk[pos + 1]) == 'r')
                *value = a >> b;
            else if (tolower(chunk[pos + 1]) == 'm' && tolower(chunk[pos + 3]) == 'n')
                *value = a < b ? a : b;
            else if (tolower(chunk[pos + 1]) == 'm' && tolower(chunk[pos + 3]) == 'x')
                *value = a > b ? a : b;
            return ret;
        }
        if (!strncasecmp(&chunk[pos], "<to_fl,", 7)) {
            uint32_t a;

            ret = legacy_parse_gen_value(chunk, pos + 7, end, &a);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            float a_fl = (float)a;
            memcpy(value, &a_fl, sizeof(uint32_t));
            return ret;
        }
        if (!strncasecmp(&chunk[pos], "<if_eq,", 7) ||
                !strncasecmp(&chunk[pos], "<if_gt,", 7) ||
                !strncasecmp(&chunk[pos], "<if_lt,", 7) ||
                !strncasecmp(&chunk[pos], "<if_ge,", 7) ||
                !strncasecmp(&chunk[pos], "<if_le,", 7)) {
            int token_pos = pos + 7;
            uint32_t a, b, c, d;

            ret = legacy_parse_gen_value(chunk, token_pos, end, &a);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            token_pos = legacy_get_next_macro_arg_pos(chunk, token_pos, end);
            ret = legacy_parse_gen_value(chunk, token_pos, end, &b);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            token_pos = legacy_get_next_macro_arg_pos(chunk, token_pos, end);
            ret = legacy_parse_gen_value(chunk, token_pos, end, &c);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            token_pos = legacy_get_next_macro_arg_pos(chunk, token_pos, end);
            ret = legacy_parse_gen_value(chunk, token_pos, end, &d);
            if (ret.code != INTP_STATUS_OK)
                return ret;

            if (tolower(chunk[pos + 4]) == 'e') {
                *value = a == b ? c : d;
            } else if (tolower(chunk[pos + 4]) == 'g') {
                if (tolower(chunk[pos + 5]) == 't') {
                    *value = a > b ? c : d;
                } else {
                    *value = a >= b ? c : d;
                }
            } else if (tolower(chunk[pos + 4]) == 'l') {
                if (tolower(chunk[pos + 5]) == 't') {
                    *value = a < b ? c : d;
                } else {
                    *value = a <= b ? c : d;
                }
            }
            return ret;
        }

        __intp_ret_status(INTP_STATUS_ERROR_SYNTAX, pos); // Invalid macro
    }

    // Regular value
    *value = strtoul(&chunk[pos], NULL, 0);
    return ret;
}
