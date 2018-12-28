#include <vitasdk.h>
#include <taihen.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "patch_gens.h"
#include "patch_tools.h"
#include "patch_hooks.h"
#include "main.h"

static VG_PatchSection g_patch_section = PATCH_NONE;
static VG_PatchType g_patch_type = PATCH_TYPE_NONE;
static uint32_t g_patch_count = 0;
static VG_FeatureState g_patch_support[] = {FT_UNSUPPORTED, FT_UNSUPPORTED, FT_UNSUPPORTED};

static uint8_t vgPatchIsGame(const char titleid[], const char self[], uint32_t nid) {
    VG_GameSupport supp = GAME_UNSUPPORTED;

    if (!strncmp(titleid, TITLEID_ANY, TITLEID_LEN) ||
            !strncmp(titleid, g_main.titleid, TITLEID_LEN)) {
        if (self[0] == '\0' || strstr(g_main.sceInfo.path, self)) {
            if (nid == NID_ANY || nid == g_main.info.module_nid) {
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

static VG_IoParseState vgPatchParseAddress(
        const char chunk[], int pos, int end,
        uint8_t *segment, uint32_t *offset) {

    char *next;

    *segment = strtoul(&chunk[pos], &next, 10); // always base 10
    if (*next != ':') {
        vgLogPrintF("[PATCH] ERROR: Syntax error: missing ':'\n");
        return IO_BAD;
    }

    while (chunk[pos] != ':' && pos < end) { pos++; }
    if (pos >= end) {
        vgLogPrintF("[PATCH] ERROR: Syntax error: missing address?\n");
        return IO_BAD;
    }

    *offset = strtoul(&chunk[pos + 1], &next, 0);
    if (!isspace(*next)) {
        vgLogPrintF("[PATCH] ERROR: Syntax error: invalid address\n");
        return IO_BAD;
    }

    //vgLogPrintF("Address: %d:0x%X\n", *segment, *offset);

    return IO_OK;
}

static VG_IoParseState vgPatchParseRepeat(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len) {

    if (chunk[pos] == '*') {
        uint8_t repeat = strtoul(&chunk[pos + 1], NULL, 10);
        if (repeat * (*patch_data_len) > PATCH_MAX_LENGTH) {
            vgLogPrintF("[PATCH] ERROR: Repeat exceeds maximum patch length!\n");
            return IO_BAD;
        }

        for (int i = 0; i < repeat; i++) {
            for (int c = 0; c < *patch_data_len; c++) {
                patch_data[i * (*patch_data_len) + c] = patch_data[c];
            }
        }
        (*patch_data_len) *= repeat;
    }

    return IO_OK;
}

static VG_IoParseState vgPatchParsePatch(const char chunk[], int pos, int end) {

    // Check for hook & apply if any
    if (chunk[pos] == '>') {
        void *hookPtr;
        uint32_t importNid;
        uint8_t shallHook = 0;

        if (vgPatchParseHook(chunk, pos, end, &importNid, &hookPtr, &shallHook)) {
            return IO_BAD;
        }

        // Apply
        if (shallHook) {
            if (g_main.hook_num >= MAX_HOOK_NUM) {
                vgLogPrintF("[PATCH] ERROR: Number of hooks exceed maximum allowed!\n");
                return IO_BAD;
            }

            vgHookFunctionImport(importNid, hookPtr);
        }

        return IO_OK;
    }

    // Regular patch
    int token_begin = pos, token_end = pos;
    uint8_t segment = 0;
    uint32_t offset = 0;
    uint8_t patch_data[PATCH_MAX_LENGTH];
    uint8_t patch_data_len = 0;

    // Parse address
    while (!isspace(chunk[token_end])) { token_end++; }
    if (vgPatchParseAddress(chunk, token_begin, token_end, &segment, &offset))
        return IO_BAD;

    // Skip leading whitespaces
    token_begin = ++token_end;
    while (isspace(chunk[token_begin])) { token_begin++; }
    while (chunk[token_end] != ')') { token_end++; }

    // Parse generator & generate patch
    if (vgPatchParseGen(chunk, token_begin, token_end, patch_data, &patch_data_len)) {
        return IO_BAD;
    }

    // Parse repeat (optional)
    token_begin = ++token_end;
    while (token_begin < end && isspace(chunk[token_begin])) { token_begin++; }
    token_end = token_begin;
    if (token_begin < end) {
        while (!isspace(chunk[token_end])) { token_end++; }
        if (vgPatchParseRepeat(chunk, token_begin, token_end, patch_data, &patch_data_len)) {
            return IO_BAD;
        }
    }
    /*
    vgLogPrintF("Patch data: ");
    for (int i = 0; i < patch_data_len; i++) {
        vgLogPrintF("%X ", patch_data[i]);
    }
    vgLogPrintF("\n");
    */

    if (g_main.inject_num >= MAX_INJECT_NUM) {
        vgLogPrintF("[PATCH] ERROR: Number of patches exceed maximum allowed!\n");
        return IO_BAD;
    }

    // Apply patch
    vgInjectData(segment, offset, patch_data, patch_data_len);

    return IO_OK;
}

static VG_IoParseState vgPatchParseSection(const char chunk[], int pos, int end) {

    if ((pos + TITLEID_LEN + 1) >= end) { // does titleid even fit?
        vgLogPrintF("[PATCH] ERROR: Invalid section [%.10s ...], pos=%d\n", &chunk[pos], pos);
        return IO_BAD;
    }

    char titleid[TITLEID_LEN + 1] = TITLEID_ANY;
    char self[SELF_LEN_MAX + 1] = SELF_ANY;
    uint32_t nid = NID_ANY;

    int separator = pos + TITLEID_LEN + 1;
    if (chunk[separator] != ']' && chunk[separator] != ',') {
        vgLogPrintF("[PATCH] ERROR: Syntax error near '%.20s', pos=%d\n", &chunk[pos], pos);
        return IO_BAD;
    }

    // TITLEID (required)
    strncpy(titleid, &chunk[pos + 1], TITLEID_LEN);

    // SELF & NID (optional)
    if (chunk[separator] == ',') {
        int nextsep = separator + 1;
        while (chunk[nextsep] != ',' && chunk[nextsep] != ']') { nextsep++; }
        if (nextsep == separator + 1) {
            vgLogPrintF("[PATCH] ERROR: Syntax error!\n");
            return IO_BAD;
        }
        
        // SELF
        strncpy(self, &chunk[separator + 1], nextsep - (separator + 1));

        // NID
        if (chunk[nextsep] == ',') {
            char *end;
            nid = strtoul(&chunk[nextsep + 1], &end, 0);
            if (*end != ']') {
                vgLogPrintF("[PATCH] ERROR: Syntax error!\n");
                return IO_BAD;
            }
        }
    }

    g_patch_count++;

    if (vgPatchIsGame(titleid, self, nid)) {
        g_patch_section = PATCH_GAME;
    } else {
        // If previous patch section didn't have any patches ->
        // we are in a combined section and we shall continue
        if (g_patch_type != PATCH_TYPE_NONE) {
            // otherwise:
            g_patch_section = PATCH_NONE;
        }
    }

    // Reset patch type
    g_patch_type = PATCH_TYPE_NONE;

#ifdef ENABLE_VERBOSE_LOGGING
    vgLogPrintF("[PATCH] Found section [%s] [%s] [0x%X]\n", titleid, self, nid);
#endif
    return IO_OK;
}

static VG_IoParseState vgPatchParsePatchType(const char chunk[], int pos, int end) {
    g_patch_type = PATCH_TYPE_OFF;

    if (!strncmp(&chunk[pos], "@FB", 3)) {
        g_patch_support[0] = FT_ENABLED;
        if (vgConfigIsFbEnabled()) {
            g_patch_type = PATCH_TYPE_FB;
        }
    } else if (!strncmp(&chunk[pos], "@IB", 3)) {
        g_patch_support[1] = FT_ENABLED;
        if (vgConfigIsIbEnabled()) {
            g_patch_type = PATCH_TYPE_IB;
        }
    } else if (!strncmp(&chunk[pos], "@FPS", 4)) {
        g_patch_support[2] = FT_ENABLED;
        if (vgConfigIsFpsEnabled()) {
            g_patch_type = PATCH_TYPE_FPS;
        }
    } else {
        g_patch_type = PATCH_TYPE_NONE;
        vgLogPrintF("[PATCH] ERROR: Unknown patch type!\n");
        return IO_BAD;
    }

    return IO_OK;
}

static VG_IoParseState vgPatchParseLine(const char chunk[], int pos, int end) {

    // Check for new section
    if (chunk[pos] == '[') {
        if (!vgPatchParseSection(chunk, pos, end))
            return IO_OK;
    }
    // On current game section
    else if (g_patch_section == PATCH_GAME) {
        // Check for new patch type
        if (chunk[pos] == '@') {
            return vgPatchParsePatchType(chunk, pos, end);
        }
        // Parse & apply patches
        else if (g_patch_type != PATCH_TYPE_OFF) {
            return vgPatchParsePatch(chunk, pos, end);
        }
    }

    return IO_OK;
}

void vgPatchParse() {
    g_patch_section = PATCH_NONE;
    g_patch_type = PATCH_TYPE_NONE;

    vgLogPrintF("[PATCH] Parsing patchlist.txt\n");
    g_main.patch_state = vgIoParse(PATCH_PATH, vgPatchParseLine);
    vgLogPrintF("[PATCH] %u total patches found in patchlist.txt\n", g_patch_count);

    vgConfigSetSupported(g_patch_support[0], g_patch_support[1], g_patch_support[2]);
}
