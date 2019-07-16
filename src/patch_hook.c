#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "patch_hook.h"
#include "main.h"

int vg_hook_sceDisplaySetFrameBuf_withWait(const SceDisplayFrameBuf *pParam, int sync) {
    int ret = TAI_CONTINUE(int, g_main.hook_ref[0], pParam, sync);
    sceDisplayWaitVblankStartMulti(2);
    return ret;
}
int vg_hook_sceCtrlReadBufferPositive_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive(port, pad_data, count);
}
int vg_hook_sceCtrlReadBufferPositive2_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive2(port, pad_data, count);
}

bool vg_hook_function_import(uint32_t nid, const void *func) {
    if (g_main.hook_num >= MAX_HOOK_NUM) {
        vg_log_printf("[HOOK] ERROR: Number of hooks exceed maximum allowed!\n");
        return false;
    }

    vg_log_printf("[HOOK] Hooking function import nid=0x%X to 0x%X\n", nid, func);

    g_main.hook[g_main.hook_num] = taiHookFunctionImport(&g_main.hook_ref[g_main.hook_num], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, nid, func);
    g_main.hook_num++;
    return true;
}

static vg_io_status_t vg_hook_parse_common(
            const char line[], uint32_t *importNid, void **hookPtr, uint8_t *shallHook) {
    vg_io_status_t ret = {IO_OK, 0, 0};

    if (!strncasecmp(&line[1], "sceDisplaySetFrameBuf_withWait", 30)) {
        *importNid = 0x7A410B64;
        *hookPtr = &vg_hook_sceDisplaySetFrameBuf_withWait;
        *shallHook = g_main.config.fps_enabled == FT_ENABLED && g_main.config.fps == FPS_30;
        return ret;
    }
    if (!strncasecmp(&line[1], "sceCtrlReadBufferPositive_peekPatched", 37)) {
        *importNid = 0x67E7AB83;
        *hookPtr = &vg_hook_sceCtrlReadBufferPositive_peekPatched;
        *shallHook = g_main.config.fps_enabled == FT_ENABLED && g_main.config.fps == FPS_60;
        return ret;
    }
    if (!strncasecmp(&line[1], "sceCtrlReadBufferPositive2_peekPatched", 38)) {
        *importNid = 0xC4226A3E;
        *hookPtr = &vg_hook_sceCtrlReadBufferPositive2_peekPatched;
        *shallHook = g_main.config.fps_enabled == FT_ENABLED && g_main.config.fps == FPS_60;
        return ret;
    }

    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, 1);
}

/**
 * Parses and applies a common hook
 */
vg_io_status_t vg_hook_parse_patch(const char line[]) {
    void *hookPtr;
    uint32_t importNid;
    uint8_t shallHook = 0;
    vg_io_status_t ret = {IO_OK, 0, 0};

    // Check for common hook
    ret = vg_hook_parse_common(line, &importNid, &hookPtr, &shallHook);
    if (ret.code != IO_OK)
        return ret;

    // Apply
    if (shallHook) {
        if (vg_hook_function_import(importNid, hookPtr))
            return ret;
        else
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, 0); // FIXME
    }

    return ret;
}
