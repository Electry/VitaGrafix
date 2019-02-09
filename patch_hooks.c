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

int vgHook_sceDisplaySetFrameBuf_withWait(const SceDisplayFrameBuf *pParam, int sync) {
    int ret = TAI_CONTINUE(int, g_main.hook_ref[0], pParam, sync);
    sceDisplayWaitVblankStartMulti(2);
    return ret;
}
int vgHook_sceCtrlReadBufferPositive_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive(port, pad_data, count);
}
int vgHook_sceCtrlReadBufferPositive2_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive2(port, pad_data, count);
}

VG_IoParseState vgPatchParseHook(
            const char chunk[], int pos, int end,
            uint32_t *importNid, void **hookPtr, uint8_t *shallHook) {

    if (!strncasecmp(&chunk[pos + 1], "sceDisplaySetFrameBuf_withWait", 30)) {
        *importNid = 0x7A410B64;
        *hookPtr = &vgHook_sceDisplaySetFrameBuf_withWait;
        *shallHook = g_main.config.fps_enabled == FT_ENABLED && g_main.config.fps == FPS_30;
        return IO_OK;
    }
    if (!strncasecmp(&chunk[pos + 1], "sceCtrlReadBufferPositive_peekPatched", 37)) {
        *importNid = 0x67E7AB83;
        *hookPtr = &vgHook_sceCtrlReadBufferPositive_peekPatched;
        *shallHook = g_main.config.fps_enabled == FT_ENABLED && g_main.config.fps == FPS_60;
        return IO_OK;
    }
    if (!strncasecmp(&chunk[pos + 1], "sceCtrlReadBufferPositive2_peekPatched", 38)) {
        *importNid = 0xC4226A3E;
        *hookPtr = &vgHook_sceCtrlReadBufferPositive2_peekPatched;
        *shallHook = g_main.config.fps_enabled == FT_ENABLED && g_main.config.fps == FPS_60;
        return IO_OK;
    }

    return IO_BAD;
}