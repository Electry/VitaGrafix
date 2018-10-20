#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "config.h"
#include "tools.h"
#include "patch.h"
#include "main.h"
#include "log.h"


VG_Main g_main = {0};

void vgInjectData(int segidx, uint32_t offset, const void *data, size_t size) {
    vgLogPrintF("Patching seg%03d : %08X to", segidx, offset);
    for (size_t i = 0; i < size; i++) {
        vgLogPrintF(" %02X", ((uint8_t *)data)[i]);
    }
    vgLogPrintF(", size=%d\n", size);

    g_main.inject[g_main.inject_num] = taiInjectData(g_main.info.modid, segidx, offset, data, size);
    g_main.inject_num++;
}
void vgHookFunction(int segidx, uint32_t offset, int thumb, const void *func) {
    vgLogPrintF("Hooking seg%03d:%08X to 0x%X, thumb=%d\n", segidx, offset, func, thumb);

    g_main.hook[g_main.hook_num] = taiHookFunctionOffset(&g_main.hook_ref[g_main.hook_num], g_main.info.modid, segidx, offset, thumb, func);
    g_main.hook_num++;
}
void vgHookFunctionImport(uint32_t nid, const void *func) {
    vgLogPrintF("Hooking function import nid=0x%X to 0x%X\n", nid, func);

    g_main.hook[g_main.hook_num] = taiHookFunctionImport(&g_main.hook_ref[g_main.hook_num], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, nid, func);
    g_main.hook_num++;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {

    if (!g_main.timer) // Start OSD timer
        g_main.timer = sceKernelGetProcessTimeLow();

    if (sceKernelGetProcessTimeLow() - g_main.timer < OSD_SHOW_DURATION) {
        osdUpdateFrameBuf(pParam);
        // Scale font size
        if (pParam->width == 640)
            osdSetTextScale(1);
        else
            osdSetTextScale(2);

        // TITLEID
        int y = 5;
        osdDrawStringF(5, y, "%s: %s", g_main.titleid,
                    g_main.config.game_enabled == FT_ENABLED ? "patched" : "disabled");

        // Patch info
        if (vgConfigIsFbEnabled()) {
            osdDrawStringF(5, y += 20, "Framebuffer: %dx%d", g_main.config.fb.width, g_main.config.fb.height);
        } else if (g_main.config.game_enabled == FT_ENABLED &&
                    g_main.config.fb_enabled != FT_UNSUPPORTED) {
            osdDrawStringF(5, y += 20, "Framebuffer: default");
        }
        if (vgConfigIsIbEnabled()) {
            char buf[64] = "";
            snprintf(buf, 64, "%dx%d", g_main.config.ib[0].width, g_main.config.ib[0].height);
            for (uint8_t i = 1; i < g_main.config.ib_count; i++) {
                snprintf(buf, 64, "%s, %dx%d", buf, g_main.config.ib[i].width, g_main.config.ib[i].height);
            }
            osdDrawStringF(5, y += 20, "Internal: %s", buf);
        } else if (g_main.config.game_enabled == FT_ENABLED &&
                    g_main.config.ib_enabled != FT_UNSUPPORTED) {
            osdDrawStringF(5, y += 20, "Internal: default");
        }
        if (vgConfigIsFpsEnabled()) {
            osdDrawStringF(5, y += 20, "FPS cap: %d", g_main.config.fps == FPS_30 ? 30 : 60);
        } else if (g_main.config.game_enabled == FT_ENABLED &&
                    g_main.config.fps_enabled != FT_UNSUPPORTED) {
            osdDrawStringF(5, y += 20, "FPS cap: default");
        }

        // Config info
        if (g_main.config_state != CONFIG_OK) {
            if (g_main.config_state == CONFIG_BAD) {
                    osdDrawStringF(pParam->width / 2 - osdGetTextWidth(MSG_CONFIG_BAD) / 2,
                                    pParam->height / 2 - 10,
                                    MSG_CONFIG_BAD);
            } else if (g_main.config_state == CONFIG_OPEN_FAILED) {
                    osdDrawStringF(pParam->width / 2 - osdGetTextWidth(MSG_CONFIG_OPEN_FAILED) / 2,
                                    pParam->height / 2 - 10,
                                    MSG_CONFIG_OPEN_FAILED);
            }
        // Game info
        } else if (g_main.support == GAME_WRONG_VERSION) {
            osdDrawStringF(pParam->width / 2 - osdGetTextWidth(MSG_GAME_WRONG_VERSION) / 2,
                            pParam->height / 2 - 10,
                            MSG_GAME_WRONG_VERSION);
        }

    } else {
        // Release no longer used hook
        int ret = TAI_CONTINUE(int, g_main.sceDisplaySetFrameBuf_hookref, pParam, sync);

        taiHookRelease(g_main.sceDisplaySetFrameBuf_hookid, g_main.sceDisplaySetFrameBuf_hookref);
        g_main.sceDisplaySetFrameBuf_hookid = -1;
        return ret;
    }

    return TAI_CONTINUE(int, g_main.sceDisplaySetFrameBuf_hookref, pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

    // Getting eboot.bin info
    g_main.info.size = sizeof(tai_module_info_t);
    taiGetModuleInfo(TAI_MAIN_MODULE, &g_main.info);

    g_main.sceInfo.size = sizeof(SceKernelModuleInfo);
    sceKernelGetModuleInfo(g_main.info.modid, &g_main.sceInfo);

    // Getting app titleid
    sceAppMgrAppParamGetString(0, 12, g_main.titleid, 16);

    // Create VG folder
    sceIoMkdir(VG_FOLDER, 0777);
    vgLogPrepare();

    vgLogPrintF("Title ID: %s\n", g_main.titleid);

    // Parse config
    vgConfigParse();
    if (g_main.config.enabled == FT_DISABLED) {
        return SCE_KERNEL_START_SUCCESS;
    }

    // Patch the game
    vgPatchGame();

    // If game is unsupported, mark it as disabled
    if (g_main.support != GAME_SUPPORTED) {
        g_main.config.game_enabled = FT_DISABLED;
    }

    // If no features are enabled, mark game as disabled
    if (g_main.support == GAME_SUPPORTED &&
            !vgConfigIsFbEnabled() &&
            !vgConfigIsIbEnabled() &&
            !vgConfigIsFpsEnabled()) {
        g_main.config.game_enabled = FT_DISABLED;
    }

    // Hook sceDisplaySetFrameBuf for OSD
    if ((g_main.support == GAME_SUPPORTED || g_main.support == GAME_WRONG_VERSION) &&
            vgConfigIsOsdEnabled()) {
        g_main.timer = 0;
        g_main.sceDisplaySetFrameBuf_hookid = taiHookFunctionImport(
                &g_main.sceDisplaySetFrameBuf_hookref,
                TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY,
                0x7A410B64,
                sceDisplaySetFrameBuf_patched);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    // Release OSD hook
    if ((g_main.support == GAME_SUPPORTED || g_main.support == GAME_WRONG_VERSION) &&
            vgConfigIsOsdEnabled() && g_main.sceDisplaySetFrameBuf_hookid >= 0) {
        taiHookRelease(g_main.sceDisplaySetFrameBuf_hookid, g_main.sceDisplaySetFrameBuf_hookref);
    }

    // Release game patches
    while (g_main.inject_num-- > 0) {
        if (g_main.inject[g_main.inject_num] >= 0)
            taiInjectRelease(g_main.inject[g_main.inject_num]);
    }
    // Release game hooks
    while (g_main.hook_num-- > 0) {
        if (g_main.hook[g_main.hook_num] >= 0)
            taiHookRelease(g_main.hook[g_main.hook_num], g_main.hook_ref[g_main.hook_num]);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
