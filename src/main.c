#include <vitasdk.h>
#include <taihen.h>
#include <stdio.h>
#include <stdbool.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "main.h"
#include "osd.h"

vg_main_t g_main = {0};

// string buffer
char g_osd_buffer[STRING_BUFFER_SIZE] = "";

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
    // OSD not shown yet? Start the timer
    if (!g_main.osd_timer) {
        g_main.osd_timer = sceKernelGetProcessTimeLow();
    }
    // OSD timer finished? Release the hook
    else if (sceKernelGetProcessTimeLow() - g_main.osd_timer > OSD_SHOW_DURATION
            && g_main.config_status.code == IO_OK // Show indefinitely on i/o error
            && g_main.patch_status.code == IO_OK) {
        int ret = TAI_CONTINUE(int, g_main.osd_hook_ref, pParam, sync);

        taiHookRelease(g_main.osd_hook, g_main.osd_hook_ref);
        g_main.osd_hook = -1;
        return ret;
    }

    osd_update_fb(pParam);
    osd_set_back_color(0, 0, 0, 200);

    // Background
    int w = 180;
    if (g_main.config.ib_count > 1)
        w += 100; // Fit "960x544 >> 720x408"
    if ((g_main.config.fb_enabled != FT_UNSUPPORTED
            || g_main.config.ib_enabled != FT_UNSUPPORTED)
            && g_main.config.msaa_enabled == FT_ENABLED)
        w += 45; // Fit "960x544 (4x)"
    if (g_main.config.fb_enabled == FT_DISABLED || g_main.config.fb_enabled == FT_UNSPECIFIED
            || g_main.config.ib_enabled == FT_DISABLED || g_main.config.ib_enabled == FT_UNSPECIFIED)
        w += 45; // Fit "Res: default"
    if ((g_main.config.fb_enabled == FT_ENABLED && g_main.config.fb.width > 999)
            || (g_main.config.ib_enabled == FT_ENABLED && g_main.config.ib[0].width > 999))
        w += 10; // Fit "1280x720"
    osd_draw_rectangle_fast(20, 20, w, 70);

    // Logo
    osd_draw_logo(30 + 5, 30); // 60x38

    // Version
    osd_set_back_color(0, 0, 0, 0);
    osd_set_text_scale_fl(0.5f);
    osd_draw_stringf(35 + 5 + 21, 70 + 2, VG_VERSION); // 30x10

    // Draw configuration
    osd_set_text_scale(1);
    int y = 56;

    // IO/parse failure?
    if (g_main.config_status.code != IO_OK || g_main.patch_status.code != IO_OK) {
        osd_draw_string(110, y, "Error");
        osd_set_back_color(0, 0, 0, 255);

        // Draw short message
        if (g_main.config_status.code == IO_ERROR_OPEN_FAILED) {
            osd_draw_string(20, 110, OSD_MSG_CONFIG_OPEN_FAILED);
            osd_draw_string(20, 130, OSD_MSG_IOPLUS_HINT);
        } else if (g_main.patch_status.code == IO_ERROR_OPEN_FAILED) {
            osd_draw_string(20, 110, OSD_MSG_PATCH_OPEN_FAILED);
            osd_draw_string(20, 130, OSD_MSG_IOPLUS_HINT);
        } else if (g_main.config_status.code != IO_OK) {
            osd_draw_string(20, 110, OSD_MSG_CONFIG_ERROR);
        } else if (g_main.patch_status.code != IO_OK) {
            osd_draw_string(20, 110, OSD_MSG_PATCH_ERROR);
        }

        // Draw first x characters from log
        if (g_main.config.log_enabled) {
            osd_draw_string(20, 150, g_osd_buffer);
        }
    }
    // Wrong version
    else if (g_main.support == GAME_WRONG_VERSION) {
        osd_set_back_color(0, 0, 0, 255);
        osd_draw_string(pParam->width / 2 - osd_get_text_width(OSD_MSG_GAME_WRONG_VERSION) / 2,
                        pParam->height / 2 - 20,
                        OSD_MSG_GAME_WRONG_VERSION);
    }
    else {
        // MSAA
        char msaa_sm_buf[16] = "";
        if (g_main.config.msaa_enabled == FT_ENABLED) {
            snprintf(msaa_sm_buf, 16, "%s",
                    (g_main.config.msaa == MSAA_4X ? "4x" :
                    (g_main.config.msaa == MSAA_2X ? "2x" : "1x")));
        }

        // 2nd line
        if (g_main.config.fps_enabled == FT_ENABLED) {
            osd_draw_stringf(110, y, "%d FPS",
                    g_main.config.fps == FPS_60 ? 60 : 30);
            y -= 20;
        } else if (g_main.config.fps_enabled != FT_UNSUPPORTED) {
            osd_draw_stringf(110, y, "FPS: default");
            y -= 20;
        }

        // 1st line
        char res_buf[32] = "";
        if (g_main.config.fb_enabled == FT_ENABLED) {
            snprintf(res_buf, 32, "%dx%d",
                    g_main.config.fb.width,
                    g_main.config.fb.height);
        } else if (g_main.config.ib_enabled == FT_ENABLED) {
            if (g_main.config.ib_count == 1) {
                snprintf(res_buf, 32, "%dx%d",
                        g_main.config.ib[0].width,
                        g_main.config.ib[0].height);
            } else {
                snprintf(res_buf, 32, "%dx%d >> %dx%d",
                        g_main.config.ib[0].width,
                        g_main.config.ib[0].height,
                        g_main.config.ib[g_main.config.ib_count - 1].width,
                        g_main.config.ib[g_main.config.ib_count - 1].height);
            }
        } else if (g_main.config.fb_enabled != FT_UNSUPPORTED
                    || g_main.config.ib_enabled != FT_UNSUPPORTED) {
            snprintf(res_buf, 32, "Res: default");
        } else if (g_main.config.msaa_enabled == FT_ENABLED) {
            snprintf(res_buf, 32, "MSAA: %s", msaa_sm_buf);
        } else if (g_main.config.msaa_enabled != FT_UNSUPPORTED) {
            snprintf(res_buf, 16, "MSAA: default");
        }

        if (res_buf[0] != '\0') {
            if (g_main.config.msaa_enabled == FT_ENABLED
                    && (g_main.config.fb_enabled != FT_UNSUPPORTED
                    || g_main.config.ib_enabled != FT_UNSUPPORTED))
                osd_draw_stringf(110, y, "%s (%s)", res_buf, msaa_sm_buf);
            else
                osd_draw_stringf(110, y, "%s", res_buf);
        }
    }

    return TAI_CONTINUE(int, g_main.osd_hook_ref, pParam, sync);
}

void vg_main_log_header() {
#ifndef ENABLE_VERBOSE_LOGGING
    // Don't overwrite log
    vg_log_prepare();
#endif
    // Log basic info
    vg_log_printf("VitaGrafix " VG_VERSION "\n");
    vg_log_printf("=======================================\n");
    vg_log_printf("[MAIN] Title ID: %s\n", g_main.titleid);
    vg_log_printf("[MAIN] SELF: %s\n", g_main.sce_info.path);
    vg_log_printf("[MAIN] NID: 0x%X\n", g_main.tai_info.module_nid);
    vg_log_printf("=======================================\n");
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    // Get app titleid
    sceAppMgrAppParamGetString(0, 12, g_main.titleid, 16);

    // Exit if using VitaShell
    if (!strncmp(g_main.titleid, "VITASHELL", TITLEID_LEN)) {
        goto EXIT;
    }

    // Get eboot.bin info
    g_main.tai_info.size = sizeof(tai_module_info_t);
    g_main.sce_info.size = sizeof(SceKernelModuleInfo);
    taiGetModuleInfo(TAI_MAIN_MODULE, &g_main.tai_info);
    sceKernelGetModuleInfo(g_main.tai_info.modid, &g_main.sce_info);

    // Create VitaGrafix folder (if doesn't exist)
    sceIoMkdir(VG_FOLDER, 0777);

#ifdef ENABLE_VERBOSE_LOGGING
    // Create log file before config is parsed
    // so we can print parsing debug info
    vg_log_prepare();
#endif

    // Parse config.txt
    vg_config_parse();
    if (g_main.config_status.code != IO_OK) {
        g_main.config.enabled = g_main.config.osd_enabled = FT_ENABLED;
        vg_main_log_header();
        vg_log_printf("[PATCH] Failed to parse config (line %d, pos %d): %s\n",
                    g_main.config_status.line,
                    g_main.config_status.pos_line,
                    vg_io_status_code_to_string(g_main.config_status.code));
    } else if (g_main.config.log_enabled == FT_ENABLED) {
        vg_main_log_header();
    }

    // Exit now?
    if (g_main.config.enabled == FT_DISABLED)
        goto EXIT;

    // Skip parsing patchlist if there was an error in config
    if (g_main.config_status.code != IO_OK)
        goto EXIT_HOOK_OSD;

    // Parse patchlist & apply patches
    vg_patch_parse_and_apply();
    if (g_main.patch_status.code != IO_OK) {
        g_main.config.osd_enabled = FT_ENABLED;
        vg_log_printf("[PATCH] Failed to parse patchlist (line %d, pos %d): %s\n",
                    g_main.patch_status.line,
                    g_main.patch_status.pos_line,
                    vg_io_status_code_to_string(g_main.patch_status.code));
        goto EXIT_HOOK_OSD;
    }

    // Exit if game is not supported / is self shell
    if (g_main.support == GAME_SELF_SHELL
                || g_main.support == GAME_UNSUPPORTED)
        goto EXIT;

EXIT_HOOK_OSD:
    // Hook sceDisplaySetFrameBuf for OSD
    if (g_main.config.osd_enabled) {
        g_main.osd_timer = 0;
        g_main.osd_hook = taiHookFunctionImport(
                    &g_main.osd_hook_ref,
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x7A410B64,
                    sceDisplaySetFrameBuf_patched);

        if (g_main.config_status.code != IO_OK || g_main.patch_status.code != IO_OK) {
            vg_log_read(g_osd_buffer, STRING_BUFFER_SIZE);
        }
    }

EXIT:
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    // Release OSD hook
    if (g_main.osd_hook >= 0) {
        taiHookRelease(g_main.osd_hook, g_main.osd_hook_ref);
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
