#include <vitasdk.h>
#include <taihen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

typedef char vg_patch_resolution_limits_must_match[MAX_RES_COUNT == INTP_VG_MAX_RES_COUNT ? 1 : -1];

static vg_patch_section_t g_patch_section       = PATCH_SECTION_NONE;
static vg_feature_t       g_patch_feature       = FEATURE_INVALID;
static uint32_t           g_patch_applied_size  = 0;
static vg_io_status_t     g_patch_status        = {0};

// Alternative patchlist path
static char g_patch_alte_path[PATCH_ALTE_PATH_LEN] = "";

// Feature support, updated according to patchlist.txt entry for current game.
// Used to update global feature support after patchlist.txt is parsed.
static vg_feature_state_t g_patch_support[FEATURE_INVALID];

static const vg_patch_feature_token_t _FEATURE_TOKENS[FEATURE_INVALID] = {
    {"@FB",   FEATURE_FB},
    {"@IB",   FEATURE_IB},
    {"@FPS",  FEATURE_FPS},
    {"@MSAA", FEATURE_MSAA}
};

static void vg_patch_set_interpreter_context() {
    const vg_config_t *config = vg_config_get();
    intp_vg_context_t context = {0};

    context.fb_width = config->fb.width;
    context.fb_height = config->fb.height;

    for (int i = 0; i < INTP_VG_MAX_RES_COUNT; i++) {
        context.ib_width[i] = config->ib[i].width;
        context.ib_height[i] = config->ib[i].height;
    }

    switch (config->fps) {
        case FPS_30:
            context.vblank = 2;
            context.fps_limit = 30;
            break;
        case FPS_20:
            context.vblank = 3;
            context.fps_limit = 20;
            break;
        case FPS_60:
        default:
            context.vblank = 1;
            context.fps_limit = 60;
            break;
    }

    // SCE_GXM_MULTISAMPLE_*
    context.msaa = config->msaa == MSAA_4X ? 2 : config->msaa == MSAA_2X ? 1 : 0;
    context.msaa_enabled = context.msaa > 0;

    intp_set_vg_context(&context);
}

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

/**
 * Parses segment & offset (e.g. 0:0x12345)
 */
static vg_io_status_t vg_patch_parse_address(const char line[], int *pos, uint8_t *segment, uint32_t *offset) {

    char *next = NULL;

    // Parse segment
    if (!isdigit(line[*pos]))
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, *pos);
    unsigned long segment_value = strtoul(&line[*pos], &next, 10); // always base 10
    if (next == &line[*pos] || segment_value > UINT8_MAX)
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, *pos);
    if (*next != ':') {
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, next - line);
    }
    *segment = segment_value;
    *pos = next - line + 1;

    // Parse offset
    if (!isdigit(line[*pos]))
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, *pos);
    unsigned long long offset_value = strtoull(&line[*pos], &next, 0);
    if (next == &line[*pos] || offset_value > UINT32_MAX)
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, *pos);
    if (!isspace(*next))
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, next - line);
    *offset = offset_value;
    *pos = next - line;

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
    vg_io_section_header_t header;

    vg_io_status_t ret = vg_io_parse_section_header(line, &header);
    if (ret.code != IO_OK)
        return ret;

    vg_module_match_t match = vg_main_match_current_module(header.titleid, header.self, header.nid, false);
    if (match > g_main.patch_match) {
        g_main.patch_match = match;
    }

    if (match == MODULE_MATCH) {
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
    vg_log_printf("[PATCH] Found section [%s] [%s] [0x%X]\n", header.titleid, header.self, header.nid);
#endif
    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_patch_parse_patch_type(const char line[]) {
    // Check for valid feature type
    for (int i = 0; i < FEATURE_INVALID; i++) {
        size_t token_length = strlen(_FEATURE_TOKENS[i].name);
        if (!strncasecmp(line, _FEATURE_TOKENS[i].name, token_length) && vg_io_is_line_end(line, token_length)) {
            g_patch_support[i] = FT_ENABLED; // mark feature as supported
            g_patch_feature = i;

            __ret_status(IO_OK, 0, 0);
        }
    }

    g_patch_feature =  FEATURE_INVALID;
    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, 0);
}

static vg_io_status_t vg_patch_parse_patch_directive(const char line[]) {
    int pos, epos;
    int llen = strlen(line);

    if (!strncasecmp(line, "!USE", 4)) {
        pos = 4;
        while (isspace(line[pos])) { pos++; }
        if (line[pos] != '(')
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

        epos = pos++;
        while (epos < llen - 1 && line[epos] != ')') { epos++; }
        if (line[epos] != ')')
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, epos);

        int path_length = epos - pos;
        if (path_length >= PATCH_ALTE_PATH_LEN)
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
        if (!vg_io_is_line_end(line, epos + 1))
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, epos + 1);

        // Copy alternative path
        memcpy(g_patch_alte_path, &line[pos], path_length);
        g_patch_alte_path[path_length] = '\0';

        __ret_status(IO_DIRECTIVE_ALTE_FILE, 0, 0);
    }

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

        // Check for patcher directive
        if (line[0] == '!') {
            return vg_patch_parse_patch_directive(line);
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

vg_io_status_t vg_patch_parse_and_apply() {
    g_patch_section = PATCH_SECTION_NONE;
    g_patch_feature = FEATURE_INVALID;

    // Reset supported features list
    for (int i = 0; i < FEATURE_INVALID; i++) {
        g_patch_support[i] = FT_UNSUPPORTED;
    }

    vg_patch_set_interpreter_context();

    SceUInt32 start = sceKernelGetProcessTimeLow();
    char path[128];

    // Try title-specific patch file in a title-prefixed patch subdirectory first
    snprintf(path, 128, "%s%.4s/%s.txt", PATCH_DIR, g_main.titleid, g_main.titleid);
    g_patch_status = vg_io_parse(path, vg_patch_parse_line, false);

    // Not found? Try title-specific patch in the patch subdirectory
    if (g_patch_status.code == IO_ERROR_OPEN_FAILED) {
        snprintf(path, 128, "%s%s.txt", PATCH_DIR, g_main.titleid);
        g_patch_status = vg_io_parse(path, vg_patch_parse_line, false);
    }

    // Doesn't exist? Read patchlist.txt
    if (g_patch_status.code == IO_ERROR_OPEN_FAILED) {
        snprintf(path, 128, "%s", PATCH_LIST_PATH);
        g_patch_status = vg_io_parse(path, vg_patch_parse_line, false);
    }

    // Read alternative patch file if directed to
    if (g_patch_status.code == IO_DIRECTIVE_ALTE_FILE) {
        snprintf(path, 128, "%s%s.txt", PATCH_DIR, g_patch_alte_path);
        vg_log_printf("[PATCH] Redirecting to %s\n", path);
        g_patch_status = vg_io_parse(path, vg_patch_parse_line, false);
    }

    SceUInt32 end = sceKernelGetProcessTimeLow();

    if (g_main.inject_num > 0) {
        vg_log_printf("[PATCH] Patched %u bytes in %d patches and it took %ums\n",
                        g_patch_applied_size, g_main.inject_num, (end - start) / 1000);
    }

    // Mark features as unsupported (those for which patches haven't been found)
    vg_config_apply_patch_capabilities(g_patch_support);

    // NOTE: Patches injected before a parse failure remain active
    return g_patch_status;
}

const vg_io_status_t *vg_patch_get_status() {
    return &g_patch_status;
}
