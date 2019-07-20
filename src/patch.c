#include <vitasdk.h>
#include <taihen.h>
#include <stdlib.h>

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

static vg_patch_section_t g_patch_section = PATCH_SECTION_NONE;
static vg_feature_t       g_patch_feature = FEATURE_INVALID;
static uint32_t           g_patch_count   = 0;

// Feature support, updated according to patchlist.txt entry for current game.
// Used to update global feature support after patchlist.txt is parsed.
static vg_feature_state_t g_patch_support[FEATURE_INVALID];

static const vg_patch_feature_token_t _FEATURE_TOKENS[FEATURE_INVALID] = {
    {"@FB",   FEATURE_FB},
    {"@IB",   FEATURE_IB},
    {"@FPS",  FEATURE_FPS},
    {"@MSAA", FEATURE_MSAA}
};

static bool vg_inject_data(int segidx, uint32_t offset, const void *data, size_t size) {
    if (g_main.inject_num >= MAX_INJECT_NUM) {
        vg_log_printf("[PATCH] ERROR: Number of patches exceed maximum allowed!\n");
        return false;
    }

    vg_log_printf("[PATCH] Patching seg%03d : %08X to", segidx, offset);
    for (size_t i = 0; i < size; i++) {
        vg_log_printf(" %02X", ((uint8_t *)data)[i]);
    }
    vg_log_printf(", size=%d\n", size);

    g_main.inject[g_main.inject_num] = taiInjectData(g_main.tai_info.modid, segidx, offset, data, size);
    g_main.inject_num++;
    return true;
}

static bool vg_patch_is_game(const char titleid[], const char self[], uint32_t nid) {
    vg_game_support_t supp = GAME_UNSUPPORTED;

    if (!strncasecmp(titleid, TITLEID_ANY, TITLEID_LEN) ||
            !strncasecmp(titleid, g_main.titleid, TITLEID_LEN)) {
        if (self[0] == '\0' || strstr(g_main.sce_info.path, self)) {
            if (nid == NID_ANY || nid == g_main.tai_info.module_nid) {
                supp = GAME_SUPPORTED;
            } else {
                supp = GAME_WRONG_VERSION;
            }
        } else {
            supp = GAME_SELF_SHELL;
        }
    }

    // Update global support
    if (supp > g_main.support)
        g_main.support = supp;

    return supp == GAME_SUPPORTED;
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
    intp_value_t patch_data;

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

    // Apply patch
    bool injected = vg_inject_data(segment, offset, patch_data.data.raw, patch_data.size);
    if (!injected)
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_patch_parse_section(const char line[]) {
    size_t len = strlen(line);
    int pos = 0;

    if (TITLEID_LEN + 1 >= len) // Line too short?
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, len);

    // Parsed values
    char titleid[TITLEID_LEN + 1] = TITLEID_ANY;
    char self[SELF_LEN_MAX + 1] = SELF_ANY;
    uint32_t nid = NID_ANY;

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
            nid = strtoul(&line[pos], &end, 0);
            if (end == &line[pos])
                __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

            pos += (end - &line[pos]);
        }
    }

    // Match closing bracket ']'
    while (isspace(line[pos])) { pos++; }
    if (line[pos] != ']')
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

    g_patch_count++;

    if (vg_patch_is_game(titleid, self, nid)) {
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

    g_main.patch_status = vg_io_parse(PATCH_PATH, vg_patch_parse_line);

    vg_log_printf("[PATCH] %u total patches found in patchlist.txt\n", g_patch_count);

    // Mark features as unsupported (those for which patches haven't been found)
    vg_config_set_unsupported_features(g_patch_support);
}
