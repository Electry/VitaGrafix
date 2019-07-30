#include <vitasdk.h>
#include <taihen.h>
#include <stdlib.h>
#include <stdio.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "patch_hook.h"
#ifdef BUILD_SIG_SUPPORT
#include "patch_sig.h"
#endif
#include "main.h"

#include "interpreter/interpreter.h"

static vg_patch_section_t g_patch_section       = PATCH_SECTION_NONE;
static vg_feature_t       g_patch_feature       = FEATURE_INVALID;
static uint32_t           g_patch_total_count   = 0;
static uint32_t           g_patch_applied_size  = 0;

// Feature support, updated according to patchlist.txt entry for current game.
// Used to update global feature support after patchlist.txt is parsed.
static vg_feature_state_t g_patch_support[FEATURE_INVALID];

static const vg_patch_feature_token_t _FEATURE_TOKENS[FEATURE_INVALID] = {
    {"@FB",   FEATURE_FB},
    {"@IB",   FEATURE_IB},
    {"@FPS",  FEATURE_FPS},
    {"@MSAA", FEATURE_MSAA}
};

static vg_io_status_t vg_inject_data(int segidx, uint32_t offset, const void *data, size_t size) {
    if (g_main.inject_num >= MAX_INJECT_NUM) {
        __ret_status(IO_ERROR_TOO_MANY_PATCHES, 0, 0);
    }

    vg_log_printf("[PATCH] Patching seg%03d : %08X to", segidx, offset);
    for (size_t i = 0; i < size; i++) {
        vg_log_printf(" %02X", ((uint8_t *)data)[i]);
    }
    vg_log_printf(", size=%d\n", size);

    g_main.inject[g_main.inject_num] = taiInjectData(g_main.tai_info.modid, segidx, offset, data, size);
    if (g_main.inject[g_main.inject_num] == 0x90010005) { // TAI_ERROR_PATCH_EXISTS
        __ret_status(IO_ERROR_TAI_PATCH_EXISTS, 0, 0);
    } else if (g_main.inject[g_main.inject_num] < 0) {
        __ret_status(IO_ERROR_TAI_GENERIC, 0, 0);
    }

    g_main.inject_num++;
    g_patch_applied_size += size;
    __ret_status(IO_OK, 0, 0);
}

static byte_t *vg_patch_get_vaddr(uint8_t segment, uint32_t offset) {
    return (byte_t *)((uint32_t)g_main.sce_info.segments[segment].vaddr + offset);
}

/**
 * Parses segment & offset (e.g. 0:0x12345)
 */
static vg_io_status_t vg_patch_parse_address(
        const char line[], int *pos, uint8_t *segment, uint32_t *offset) {

    char *next = NULL;

    // Parse segment
    *segment = strtoul(&line[*pos], &next, 10); // always base 10
    (*pos) += (next - &line[*pos]) + 1;
    if (*next != ':') {
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, (*pos - 1));
    }

    // Parse offset
    *offset = strtoul(&line[*pos], &next, 0);
    (*pos) += (next - &line[*pos]) + 1;
    if (!isspace(*next))
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, (*pos - 1));

    //vg_log_printf("Address: %d:0x%X\n", *segment, *offset);

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_patch_parse_patch(const char line[]) {
    uint8_t segment = 0;
    uint32_t offset = 0;
    intp_value_t patch_data = {0};

    vg_io_status_t ret;
    int pos = 0;

    // Parse address
    ret = vg_patch_parse_address(line, &pos, &segment, &offset);
    if (ret.code != IO_OK)
        return ret;

    while (isspace(line[pos])) { pos++; }

    // Evaluate expression
    intp_status_t intp_ret = intp_evaluate(line, (uint32_t *)&pos, &patch_data);
    if (intp_ret.code != INTP_STATUS_OK) {
        // Log error info
        char buf[256];
        intp_format_error(line, intp_ret, buf, 256);
        vg_log_printf("%s\n", buf);

        __ret_status(IO_ERROR_INTERPRETER_ERROR, 0, intp_ret.pos);
    }

    for (byte_t i = 0; i < patch_data.size; i++) {
        byte_t patch_end_i = patch_data.size;

        // Skip gap
        while (i < patch_data.size && patch_data.unk[i]) { i++; }
        if (i >= patch_data.size)
            break;

        // Find next gap
        for (byte_t j = i; j < patch_data.size; j++) {
            if (patch_data.unk[j]) {
                patch_end_i = j;
                break;
            }
        }

        // Apply patch
        ret = vg_inject_data(segment, offset + i, &patch_data.data.raw[i], patch_end_i - i);
        if (ret.code != IO_OK) {
            ret.pos_line = pos; // Update pos
            return ret;
        }

        // Skip patch
        i = patch_end_i;
    }

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_patch_parse_section(const char line[]) {
    // Parsed values
    char titleid[TITLEID_LEN + 1] = TITLEID_ANY;
    char self[SELF_LEN_MAX + 1] = SELF_ANY;
    uint32_t nid = NID_ANY;

    vg_io_status_t ret = vg_io_parse_section_header(line, titleid, self, &nid);
    if (ret.code != IO_OK)
        return ret;

    g_patch_total_count++;

    if (vg_main_is_game(titleid, self, nid)) {
        g_patch_section = PATCH_SECTION_GAME;
    } else {
        // If previous patch section didn't have any patches ->
        // we are in a combined section and we shall continue
        if (g_patch_feature != FEATURE_INVALID) {
            // otherwise:
            g_patch_section = PATCH_SECTION_NONE;
        }
    }

    // Reset patch type
    g_patch_feature = FEATURE_INVALID;

#ifdef ENABLE_VERBOSE_LOGGING
    vg_log_printf("[PATCH] Found section [%s] [%s] [0x%X]\n", titleid, self, nid);
#endif
    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_patch_parse_patch_type(const char line[]) {
    // Check for valid feature type
    for (int i = 0; i < FEATURE_INVALID; i++) {
        if (!strncasecmp(line, _FEATURE_TOKENS[i].name, strlen(_FEATURE_TOKENS[i].name))) {
            g_patch_support[i] = FT_ENABLED; // mark feature as supported
            g_patch_feature = i;

            __ret_status(IO_OK, 0, 0);
        }
    }

    g_patch_feature =  FEATURE_INVALID;
    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, 0);
}

static vg_io_status_t vg_patch_parse_line(const char line[]) {
    // Check for new section
    if (line[0] == '[') {
        return vg_patch_parse_section(line);
    }

    // On current game section
    if (g_patch_section == PATCH_SECTION_GAME) {
        // Check for new feature type
        if (line[0] == '@') {
            return vg_patch_parse_patch_type(line);
        }

        // Parse & apply ENABLED patches/hooks
        if (g_patch_feature != FEATURE_INVALID
                && vg_config_is_feature_enabled(g_patch_feature)) {

            // Parse hook
            if (line[0] == '>') {
                return vg_hook_parse_patch(line);
            }

#ifdef BUILD_SIG_SUPPORT
            // Parse signature patch
            if (line[0] == '$') {
                return vg_sig_parse_patch(line);
            }
#endif

            // Parse patch
            return vg_patch_parse_patch(line);
        }
    }

    __ret_status(IO_OK, 0, 0);
}

void vg_patch_parse_and_apply() {
    g_patch_section = PATCH_SECTION_NONE;
    g_patch_feature = FEATURE_INVALID;

    // Reset supported features list
    for (int i = 0; i < FEATURE_INVALID; i++) {
        g_patch_support[i] = FT_UNSUPPORTED;
    }

    vg_log_printf("[PATCH] Parsing patchlist.txt\n");
    SceUInt32 start = sceKernelGetProcessTimeLow();

    char path[128];
    snprintf(path, 128, "%s/%s.txt", PATCH_FOLDER, g_main.titleid);

    // Try game-specific patch file
    g_main.patch_status = vg_io_parse(path, vg_patch_parse_line, false);
    if (g_main.patch_status.code == IO_ERROR_OPEN_FAILED) {
        // Doesn't exist? Read patchlist.txt
        g_main.patch_status = vg_io_parse(PATCH_LIST_PATH, vg_patch_parse_line, true);
    }

    SceUInt32 end = sceKernelGetProcessTimeLow();
    vg_log_printf("[PATCH] Patched %u bytes in %d patches and it took %ums\n",
                    g_patch_applied_size, g_main.inject_num, (end - start) / 1000);
    vg_log_printf("[PATCH] %u total patches found in patchlist.txt\n", g_patch_total_count);

    // Mark features as unsupported (those for which patches haven't been found)
    vg_config_set_unsupported_features(g_patch_support);
}
