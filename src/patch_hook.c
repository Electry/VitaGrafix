#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>
#include <strings.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "patch_hook.h"
#include "main.h"

int vg_hook_sceDisplaySetFrameBuf_withWait(const SceDisplayFrameBuf *pParam, int sync) {
    int ret = TAI_CONTINUE(int, g_main.hook_ref[HOOK_DISPLAY_SET_FRAMEBUF_WITH_WAIT], pParam, sync);
    sceDisplayWaitVblankStartMulti(2);
    return ret;
}

int vg_hook_sceCtrlReadBufferPositive_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive(port, pad_data, count);
}

int vg_hook_sceCtrlReadBufferPositive2_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive2(port, pad_data, count);
}

static vg_io_status_t vg_hook_function_import(vg_hook_id_t hook_id, uint32_t nid, const void *func) {
    vg_log_printf("[HOOK] Hooking function import nid=0x%X to 0x%X\n", nid, func);

    g_main.hook[hook_id] = taiHookFunctionImport(&g_main.hook_ref[hook_id], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, nid, func);
    if (g_main.hook[hook_id] < 0) {
        __ret_status(IO_ERROR_TAI_GENERIC, 0, 0);
    }

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_hook_parse_common(const char line[], vg_hook_id_t *hook_id,
        uint32_t *import_nid, void **hook_ptr, uint8_t *shall_hook) {
    vg_io_status_t ret = {IO_OK, 0, 0};
    const vg_config_t *config = vg_config_get();

    if (!strncasecmp(&line[1], "sceDisplaySetFrameBuf_withWait", 30)) {
        *hook_id = HOOK_DISPLAY_SET_FRAMEBUF_WITH_WAIT;
        *import_nid = 0x7A410B64;
        *hook_ptr = &vg_hook_sceDisplaySetFrameBuf_withWait;
        *shall_hook = config->fps_enabled == FT_ENABLED && config->fps == FPS_30;
        return ret;
    }
    if (!strncasecmp(&line[1], "sceCtrlReadBufferPositive_peekPatched", 37)) {
        *hook_id = HOOK_CTRL_READ_BUFFER_POSITIVE;
        *import_nid = 0x67E7AB83;
        *hook_ptr = &vg_hook_sceCtrlReadBufferPositive_peekPatched;
        *shall_hook = config->fps_enabled == FT_ENABLED && config->fps == FPS_60;
        return ret;
    }
    if (!strncasecmp(&line[1], "sceCtrlReadBufferPositive2_peekPatched", 38)) {
        *hook_id = HOOK_CTRL_READ_BUFFER_POSITIVE2;
        *import_nid = 0xC4226A3E;
        *hook_ptr = &vg_hook_sceCtrlReadBufferPositive2_peekPatched;
        *shall_hook = config->fps_enabled == FT_ENABLED && config->fps == FPS_60;
        return ret;
    }

    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, 1);
}

/**
 * Parses and applies a common hook
 */
vg_io_status_t vg_hook_parse_patch(const char line[]) {
    vg_hook_id_t hook_id;
    void *hook_ptr;
    uint32_t import_nid;
    uint8_t shall_hook = 0;
    vg_io_status_t ret = {IO_OK, 0, 0};

    // Check for common hook
    ret = vg_hook_parse_common(line, &hook_id, &import_nid, &hook_ptr, &shall_hook);
    if (ret.code != IO_OK)
        return ret;

    // Apply
    if (shall_hook)
        return vg_hook_function_import(hook_id, import_nid, hook_ptr);

    return ret;
}
