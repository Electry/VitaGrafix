#include <vitasdk.h>
#include <taihen.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "patch_gens.h"
#include "patch_tools.h"
#include "main.h"

VG_IoParseState vgPatchParseGenValue(
        const char chunk[], int pos, int end,
        uint32_t *value) {

    // Check for macros
    if (chunk[pos] == '<') {
        if (!strncmp(&chunk[pos], "<fb_w>", 6)) {
            *value = g_main.config.fb.width;
            return IO_OK;
        }
        if (!strncmp(&chunk[pos], "<fb_h>", 6)) {
            *value = g_main.config.fb.height;
            return IO_OK;
        }
        if (!strncmp(&chunk[pos], "<ib_w", 5)) {
            if (chunk[pos + 5] == '>') {
                *value = g_main.config.ib[0].width;
                return IO_OK;
            }
            else if (chunk[pos + 5] == ',') {
                uint8_t ib_n = strtoul(&chunk[pos + 6], NULL, 10);
                if (ib_n >= MAX_RES_COUNT) {
                    vgLogPrintF("[PATCH] ERROR: Accessed [%u] IB res out of range!\n", ib_n);
                    return IO_BAD;
                }

                vgConfigSetSupportedIbCount(ib_n + 1); // Raise supp. IB count
                *value = g_main.config.ib[ib_n].width;
                return IO_OK;
            }
        }
        if (!strncmp(&chunk[pos], "<ib_h", 5)) {
            if (chunk[pos + 5] == '>') {
                *value = g_main.config.ib[0].height;
                return IO_OK;
            }
            else if (chunk[pos + 5] == ',') {
                uint8_t ib_n = strtoul(&chunk[pos + 6], NULL, 10);
                if (ib_n >= MAX_RES_COUNT) {
                    vgLogPrintF("[PATCH] ERROR: Accessed [%u] IB res out of range!\n", ib_n);
                    return IO_BAD;
                }

                vgConfigSetSupportedIbCount(ib_n + 1); // Raise supp. IB count
                *value = g_main.config.ib[ib_n].height;
                return IO_OK;
            }
        }
        if (!strncmp(&chunk[pos], "<vblank>", 8)) {
            *value = g_main.config.fps == FPS_60 ? 1 : 2;
            return IO_OK;
        }
        if (!strncmp(&chunk[pos], "<+,", 3) ||
                !strncmp(&chunk[pos], "<-,", 3) ||
                !strncmp(&chunk[pos], "<*,", 3) ||
                !strncmp(&chunk[pos], "</,", 3) ||
                !strncmp(&chunk[pos], "<&,", 3) ||
                !strncmp(&chunk[pos], "<|,", 3) ||
                !strncmp(&chunk[pos], "<l,", 3) ||
                !strncmp(&chunk[pos], "<r,", 3)) {
            int token_pos = pos + 3;
            int inner_open = 0;
            uint32_t a, b;

            if (vgPatchParseGenValue(chunk, token_pos, end, &a))
                return IO_BAD;

            while (token_pos < end - 2 && (inner_open > 0 || chunk[token_pos] != ',')) {
                // Allow stacking, e.g.: </,<*,<ib_w>,10>,10>
                if (chunk[token_pos] == '<')
                    inner_open++;
                if (chunk[token_pos] == '>')
                    inner_open--;
                token_pos++;
            }
            if (vgPatchParseGenValue(chunk, token_pos + 1, end, &b))
                return IO_BAD;

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
            else if (chunk[pos + 1] == 'l')
                *value = a << b;
            else if (chunk[pos + 1] == 'r')
                *value = a >> b;
            return IO_OK;
        }

        vgLogPrintF("[PATCH] ERROR: Invalid macro!\n");
        return IO_BAD; // Invalid macro
    }

    // Regular value
    *value = strtoul(&chunk[pos], NULL, 0);
    return IO_OK;
}

VG_IoParseState vgPatchParseGen(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!vgPatchParseGen_uint16(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_uint32(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_fl32(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_bytes(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_nop(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_bkpt(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_a1_mov(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_t1_mov(chunk, pos, end, patch_data, patch_data_len) ||
            !vgPatchParseGen_t2_mov(chunk, pos, end, patch_data, patch_data_len)) {
        return IO_OK;
    }

    vgLogPrintF("[PATCH] ERROR: Invalid generator!\n");
    return IO_BAD;
}

/**
 * uint16(255)    -> FF 00
 * uint16(321)    -> 41 01
 * uint16(0x4422) -> 22 44
 */
VG_IoParseState vgPatchParseGen_uint16(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "uint16", 6)) {
        int token_end = pos;
        uint32_t value = 0;
        while (chunk[token_end] != ')') { token_end++; }

        if (vgPatchParseGenValue(chunk, pos + 7, token_end, &value))
            return IO_BAD;

        *patch_data_len = 2;
        patch_data[0] = value & 0xFF;
        patch_data[1] = (value >> 8) & 0xFF;
        return IO_OK;
    }

    return IO_BAD;
}

/**
 * uint32(960)        -> C0 03 00 00
 * uint32(0x81234567) -> 67 45 23 81
 */
VG_IoParseState vgPatchParseGen_uint32(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "uint32", 6)) {
        int token_end = pos;
        uint32_t value = 0;
        while (chunk[token_end] != ')') { token_end++; }

        if (vgPatchParseGenValue(chunk, pos + 7, token_end, &value))
            return IO_BAD;

        *patch_data_len = 4;
        patch_data[0] = value & 0xFF;
        patch_data[1] = (value >> 8) & 0xFF;
        patch_data[2] = (value >> 16) & 0xFF;
        patch_data[3] = (value >> 24) & 0xFF;
        return IO_OK;
    }

    return IO_BAD;
}

/**
 * fl32(960) -> 00 00 70 44
 * fl32(840) -> 00 00 52 44 
 */
VG_IoParseState vgPatchParseGen_fl32(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "fl32", 4)) {
        int token_end = pos;
        uint32_t value = 0;
        while (chunk[token_end] != ')') { token_end++; }

        if (vgPatchParseGenValue(chunk, pos + 5, token_end, &value))
            return IO_BAD;

        *patch_data_len = 4;
        float flvalue = (float)value;
        memcpy(&value, &flvalue, sizeof(uint32_t));
        patch_data[0] = value & 0xFF;
        patch_data[1] = (value >> 8) & 0xFF;
        patch_data[2] = (value >> 16) & 0xFF;
        patch_data[3] = (value >> 24) & 0xFF;
        return IO_OK;
    }

    return IO_BAD;
}

/**
 * bytes(DEAD)        -> DE AD
 * bytes(01020304)    -> 01 02 03 04
 * bytes(BE EF DE AD) -> BE AF DE AD
 */
VG_IoParseState vgPatchParseGen_bytes(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "bytes", 5)) {
        int token_end = pos;
        uint32_t value;

        *patch_data_len = 0;
        token_end += 6;

        while (*patch_data_len < PATCH_MAX_LENGTH && token_end < end && chunk[token_end] != ')') {
            while (isspace(chunk[token_end])) { token_end++; }

            char byte[3] = ""; // take only one byte at a time
            memcpy(&byte, &chunk[token_end], 2);
            value = strtoul(byte, NULL, 16);
            token_end += 2;

            patch_data[*patch_data_len] = value & 0xFF;
            (*patch_data_len)++;

            while (isspace(chunk[token_end])) { token_end++; }
        }

        if (*patch_data_len == PATCH_MAX_LENGTH && chunk[token_end] != ')') {
            vgLogPrintF("[PATCH] ERROR: Patch too long!\n");
            return IO_BAD;
        }

        return IO_OK;
    }

    return IO_BAD;
}

/**
 * nop() -> 00 BF
 */
VG_IoParseState vgPatchParseGen_nop(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "nop", 3)) {
        *patch_data_len = 2;
        patch_data[0] = 0x00;
        patch_data[1] = 0xBF;
        return IO_OK;
    }

    return IO_BAD;
}

/**
 * bkpt() -> 00 BE
 */
VG_IoParseState vgPatchParseGen_bkpt(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "bkpt", 4)) {
        *patch_data_len = 2;
        patch_data[0] = 0x00;
        patch_data[1] = 0xBE;
        return IO_OK;
    }

    return IO_BAD;
}

/**
 * a1_mov()
 */
VG_IoParseState vgPatchParseGen_a1_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "a1_mov", 6)) {
        int token_end = pos;
        uint32_t value;

        *patch_data_len = 4;
        uint32_t setflags = 0;
        uint32_t reg = 0;

        while (chunk[token_end] != ',') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos + 7, token_end, &setflags))
            return IO_BAD;

        token_end++;
        pos = token_end;
        while (chunk[token_end] != ',') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos, token_end, &reg))
            return IO_BAD;
            
        token_end++;
        pos = token_end;
        while (chunk[token_end] != ')') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos, token_end, &value))
            return IO_BAD;
            
        vgMakeArm_A1_MOV(reg, setflags, value, patch_data);
        return IO_OK;
    }

    return IO_BAD;
}

VG_IoParseState vgPatchParseGen_t1_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "t1_mov", 6)) {
        int token_end = pos;
        uint32_t value;

        *patch_data_len = 2;
        uint32_t reg = 0;

        while (chunk[token_end] != ',') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos + 7, token_end, &reg))
            return IO_BAD;

        token_end++;
        pos = token_end;
        while (chunk[token_end] != ')') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos, token_end, &value))
            return IO_BAD;

        vgMakeThumb_T1_MOV(reg, value, patch_data);
        return IO_OK;
    }

    return IO_BAD;
}

VG_IoParseState vgPatchParseGen_t2_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (!strncmp(&chunk[pos], "t2_mov", 6)) {
        int token_end = pos;
        uint32_t value;

        *patch_data_len = 4;
        uint32_t setflags = 0;
        uint32_t reg = 0;

        while (chunk[token_end] != ',') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos + 7, token_end, &setflags))
            return IO_BAD;

        token_end++;
        pos = token_end;
        while (chunk[token_end] != ',') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos, token_end, &reg))
            return IO_BAD;
            
        token_end++;
        pos = token_end;
        while (chunk[token_end] != ')') { token_end++; }
        if (vgPatchParseGenValue(chunk, pos, token_end, &value))
            return IO_BAD;

        vgMakeThumb2_T2_MOV(reg, setflags, value, patch_data);
        return IO_OK;
    }

    return IO_BAD;
}