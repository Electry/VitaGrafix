#include <vitasdk.h>
#include <taihen.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "main.h"
#include "osd.h"

VG_Main g_main = {0};

void vgInjectData(int segidx, uint32_t offset, const void *data, size_t size) {
    vgLogPrintF("[MAIN] Patching seg%03d : %08X to", segidx, offset);
    for (size_t i = 0; i < size; i++) {
        vgLogPrintF(" %02X", ((uint8_t *)data)[i]);
    }
    vgLogPrintF(", size=%d\n", size);

    g_main.inject[g_main.inject_num] = taiInjectData(g_main.info.modid, segidx, offset, data, size);
    g_main.inject_num++;
}
void vgHookFunction(int segidx, uint32_t offset, int thumb, const void *func) {
    vgLogPrintF("[MAIN] Hooking seg%03d:%08X to 0x%X, thumb=%d\n", segidx, offset, func, thumb);

    g_main.hook[g_main.hook_num] = taiHookFunctionOffset(&g_main.hook_ref[g_main.hook_num], g_main.info.modid, segidx, offset, thumb, func);
    g_main.hook_num++;
}
void vgHookFunctionImport(uint32_t nid, const void *func) {
    vgLogPrintF("[MAIN] Hooking function import nid=0x%X to 0x%X\n", nid, func);

    g_main.hook[g_main.hook_num] = taiHookFunctionImport(&g_main.hook_ref[g_main.hook_num], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, nid, func);
    g_main.hook_num++;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {

    if (!g_main.timer) {
        // Start OSD timer
        g_main.timer = sceKernelGetProcessTimeLow();
    }
    else if (sceKernelGetProcessTimeLow() - g_main.timer > OSD_SHOW_DURATION) {
        // Release OSD hook
        int ret = TAI_CONTINUE(int, g_main.sceDisplaySetFrameBuf_hookref, pParam, sync);

        taiHookRelease(g_main.sceDisplaySetFrameBuf_hookid, g_main.sceDisplaySetFrameBuf_hookref);
        g_main.sceDisplaySetFrameBuf_hookid = -1;
        return ret;
    }

    osdUpdateFrameBuf(pParam);
    osdSetBgColor(0, 0, 0, 200);
    if (g_main.config.ib_count > 1) {
        osdFastDrawRectangle(20, 20, 340, 70);
    } else {
        osdFastDrawRectangle(20, 20, 260, 70);
    }

    osdDrawLogo(30, 30);

    osdSetBgColor(0, 0, 0, 0);
    osdSetTextScale(1);
    osdDrawStringF(120, 30, "v4.0 ALPHA [%s]", g_main.titleid);
    osdSetTextScale(2);

    int y = 62;
    if (g_main.config.game_enabled != FT_ENABLED ||
            g_main.support == GAME_WRONG_VERSION ||
            g_main.config_state != IO_OK ||
            g_main.patch_state != IO_OK) {
        osdDrawStringF(120, y, "Disabled");
    } else {
        // FPS cap patched
        if (vgConfigIsFpsEnabled()) {
            osdDrawStringF(120, y, "%d FPS", g_main.config.fps == FPS_30 ? 30 : 60);
            y -= 20;
        }
        // FPS cap unpatched
        else if (g_main.config.game_enabled == FT_ENABLED &&
                    g_main.config.fps_enabled != FT_UNSUPPORTED) {
            osdDrawStringF(120, y, "FPS: default");
            y -= 20;
        }

        // Framebuffer resolution patched
        if (vgConfigIsFbEnabled()) {
            osdDrawStringF(120, y, "%dx%d", g_main.config.fb.width, g_main.config.fb.height);
        }
        // Internal buffer resolution patched
        else if (vgConfigIsIbEnabled()) {
            char buf[16] = "";
            if (g_main.config.ib_count > 1) {
                snprintf(buf, 16, " >> %dx%d",
                            g_main.config.ib[g_main.config.ib_count - 1].width,
                            g_main.config.ib[g_main.config.ib_count - 1].height);
            }
            osdDrawStringF(120, y, "%dx%d%s", g_main.config.ib[0].width, g_main.config.ib[0].height, buf);
        }
        // Resolution unpatched
        else if (g_main.config.game_enabled == FT_ENABLED &&
                    (g_main.config.fb_enabled != FT_UNSUPPORTED ||
                    g_main.config.ib_enabled != FT_UNSUPPORTED)) {
            osdDrawStringF(120, y, "Res: default");
        }
    }

    osdSetBgColor(0, 0, 0, 200);
    // Config info
    if (g_main.config_state != IO_OK) {
        if (g_main.config_state == IO_BAD) {
                osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_CONFIG_BAD) / 2,
                                pParam->height / 2 - 10,
                                OSD_MSG_CONFIG_BAD);
        } else if (g_main.config_state == IO_OPEN_FAILED) {
                osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_CONFIG_OPEN_FAILED) / 2,
                                pParam->height / 2 - 20,
                                OSD_MSG_CONFIG_OPEN_FAILED);
                osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_IOPLUS_HINT) / 2,
                                pParam->height / 2,
                                OSD_MSG_IOPLUS_HINT);
        }
    }
    // Patchlist info
    else if (g_main.patch_state != IO_OK) {
        if (g_main.patch_state == IO_BAD) {
                osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_PATCH_BAD) / 2,
                                pParam->height / 2 - 20,
                                OSD_MSG_PATCH_BAD);
                osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_PATCH_BAD_2) / 2,
                                pParam->height / 2,
                                OSD_MSG_PATCH_BAD_2);
        } else if (g_main.patch_state == IO_OPEN_FAILED) {
                osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_PATCH_OPEN_FAILED) / 2,
                                pParam->height / 2 - 20,
                                OSD_MSG_PATCH_OPEN_FAILED);
                osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_IOPLUS_HINT) / 2,
                                pParam->height / 2,
                                OSD_MSG_IOPLUS_HINT);
        }
    }
    // Game info
    else if (g_main.support == GAME_WRONG_VERSION) {
        osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_GAME_WRONG_VERSION) / 2,
                        pParam->height / 2 - 20,
                        OSD_MSG_GAME_WRONG_VERSION);
        osdDrawStringF(pParam->width / 2 - osdGetTextWidth(OSD_MSG_GAME_WRONG_VERSION_2) / 2,
                        pParam->height / 2,
                        OSD_MSG_GAME_WRONG_VERSION_2);
    }

    return TAI_CONTINUE(int, g_main.sceDisplaySetFrameBuf_hookref, pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

    // Get eboot.bin info
    g_main.info.size = sizeof(tai_module_info_t);
    g_main.sceInfo.size = sizeof(SceKernelModuleInfo);
    taiGetModuleInfo(TAI_MAIN_MODULE, &g_main.info);
    sceKernelGetModuleInfo(g_main.info.modid, &g_main.sceInfo);

    // Get app titleid
    sceAppMgrAppParamGetString(0, 12, g_main.titleid, 16);

    // Exit if using VitaShell
    if (!strncmp(g_main.titleid, "VITASHELL", TITLEID_LEN)) {
        return SCE_KERNEL_START_SUCCESS;
    }

    // Create VG folder
    sceIoMkdir(VG_FOLDER, 0777);
    vgLogPrepare();

    vgLogPrintF("[MAIN] Title ID: %s\n", g_main.titleid);
    vgLogPrintF("=======================================\n");

    // Parse config
    vgConfigParse();
    if (g_main.config.enabled == FT_DISABLED) {
        return SCE_KERNEL_START_SUCCESS;
    }

    // Parse patchlist
    vgPatchParse();

    // Hook sceDisplaySetFrameBuf for OSD
    if (((g_main.support == GAME_SUPPORTED || g_main.support == GAME_WRONG_VERSION) &&
            vgConfigIsOsdEnabled()) || g_main.config_state != IO_OK || g_main.patch_state != IO_OK) {
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
    if (g_main.sceDisplaySetFrameBuf_hookid >= 0) {
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
