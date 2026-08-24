#include <vitasdk.h>
#include <taihen.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "patch.h"
#include "main.h"
#include "osd.h"

vg_main_t g_main = {0};

// string buffer
char g_osd_buffer[STRING_BUFFER_SIZE] = "";

static int vg_main_get_osd_width() {
    const vg_config_t config = *vg_config_get();

    // TODO: Maybe just snprintf both lines to buf, and then osd_get_text_width(buf) ¯\_(ツ)_/¯
    int w = 180;      // Fit "960x544" or "60 FPS"

    if (config.fb_enabled == FT_UNSUPPORTED && config.ib_enabled == FT_UNSUPPORTED
            && (config.msaa_enabled == FT_DISABLED || config.msaa_enabled == FT_UNSPECIFIED)) {
        w += 60;      // Fit "MSAA: default"
    } else if (config.fb_enabled == FT_DISABLED || config.fb_enabled == FT_UNSPECIFIED
            || config.ib_enabled == FT_DISABLED || config.ib_enabled == FT_UNSPECIFIED
            || config.fps_enabled == FT_DISABLED || config.fps_enabled == FT_UNSPECIFIED) {
        w += 50;      // Fit "Res: default" or "FPS: default"
    } else if (config.fb_enabled == FT_UNSUPPORTED && config.ib_enabled == FT_UNSUPPORTED
            && config.msaa_enabled == FT_ENABLED) {
        w += 10;      // Fit "MSAA: 4x"
    }

    if (config.ib_enabled == FT_ENABLED) {
        if (config.ib[0].width > 999)
            w += 10;  // Fit "1280x720"
        if (config.ib_count > 1)
            w += 110; // Fit "960x544 >> 720x408"
    }
    if ((config.fb_enabled != FT_UNSUPPORTED || config.ib_enabled != FT_UNSUPPORTED)
            && !((config.fb_enabled == FT_ENABLED || config.ib_enabled == FT_ENABLED)           // "960x544 (4x)"
                && (config.fps_enabled == FT_DISABLED || config.fps_enabled == FT_UNSPECIFIED)) // "FPS: default"
            && config.msaa_enabled == FT_ENABLED) {
        w += 50;      // Fit "960x544 (4x)" or "Res: default (4x)"
    }

    return w;
}

static int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
    const vg_config_t config = *vg_config_get();
    const vg_io_status_t config_status = *vg_config_get_status();
    const vg_io_status_t patch_status = *vg_patch_get_status();

    // OSD not shown yet? Start the timer
    if (!g_main.osd_timer) {
        g_main.osd_timer = sceKernelGetProcessTimeLow();
    }
    // OSD timer finished? Release the hook
    else if (sceKernelGetProcessTimeLow() - g_main.osd_timer > OSD_SHOW_DURATION
            && config_status.code == IO_OK // Show indefinitely on i/o error
            && patch_status.code == IO_OK) {
        int ret = TAI_CONTINUE(int, g_main.osd_hook_ref, pParam, sync);

        taiHookRelease(g_main.osd_hook, g_main.osd_hook_ref);
        g_main.osd_hook = -1;
        return ret;
    }

    osd_update_fb(pParam);
    osd_set_back_color(0, 0, 0, 200);

    // Background
    osd_draw_rectangle_fast(20, 20, vg_main_get_osd_width(), 70);

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
    if (config_status.code != IO_OK || patch_status.code != IO_OK) {
        osd_draw_string(110, y, "Error");
        osd_set_back_color(0, 0, 0, 255);

        // Draw short message
        if (config_status.code == IO_ERROR_OPEN_FAILED) {
            osd_draw_string(20, 110, OSD_MSG_CONFIG_OPEN_FAILED);
            osd_draw_string(20, 130, OSD_MSG_IOPLUS_HINT);
        } else if (patch_status.code == IO_ERROR_OPEN_FAILED) {
            osd_draw_string(20, 110, OSD_MSG_PATCH_OPEN_FAILED);
            osd_draw_string(20, 130, OSD_MSG_IOPLUS_HINT);
        } else if (config_status.code != IO_OK) {
            osd_draw_string(20, 110, OSD_MSG_CONFIG_ERROR);
        } else if (patch_status.code != IO_OK) {
            osd_draw_string(20, 110, OSD_MSG_PATCH_ERROR);
        }

        // Draw first x characters from log
        if (config.log_enabled) {
            osd_draw_log(20, 150, pParam->height, g_osd_buffer);
        }
    }
    // Wrong version
    else if (g_main.patch_match == MODULE_NID_MISMATCH) {
        osd_draw_string(110, y, "Error");
        osd_set_back_color(0, 0, 0, 255);
        osd_draw_string(pParam->width / 2 - osd_get_text_width(OSD_MSG_GAME_WRONG_VERSION) / 2,
                        pParam->height / 2 - 20,
                        OSD_MSG_GAME_WRONG_VERSION);
    }
    else {
        // MSAA
        char msaa_sm_buf[16] = "";
        if (config.msaa_enabled == FT_ENABLED) {
            snprintf(msaa_sm_buf, 16, "%s",
                    (config.msaa == MSAA_4X ? "4x" :
                    (config.msaa == MSAA_2X ? "2x" : "1x")));
        }

        // 2nd line
        if (config.fps_enabled == FT_ENABLED) {
            osd_draw_stringf(110, y, "%d FPS",
                    config.fps == FPS_60 ? 60 : 30);
            y -= 20;
        } else if (config.fps_enabled != FT_UNSUPPORTED) {
            osd_draw_stringf(110, y, "FPS: default");
            y -= 20;
        }

        // 1st line
        char res_buf[32] = "";
        if (config.fb_enabled == FT_ENABLED) {
            snprintf(res_buf, 32, "%dx%d",
                    config.fb.width,
                    config.fb.height);
        } else if (config.ib_enabled == FT_ENABLED) {
            if (config.ib_count == 1) {
                snprintf(res_buf, 32, "%dx%d",
                        config.ib[0].width,
                        config.ib[0].height);
            } else {
                snprintf(res_buf, 32, "%dx%d >> %dx%d",
                        config.ib[0].width,
                        config.ib[0].height,
                        config.ib[config.ib_count - 1].width,
                        config.ib[config.ib_count - 1].height);
            }
        } else if (config.fb_enabled != FT_UNSUPPORTED
                    || config.ib_enabled != FT_UNSUPPORTED) {
            snprintf(res_buf, 32, "Res: default");
        } else if (config.msaa_enabled == FT_ENABLED) {
            snprintf(res_buf, 32, "MSAA: %s", msaa_sm_buf);
        } else if (config.msaa_enabled != FT_UNSUPPORTED) {
            snprintf(res_buf, 16, "MSAA: default");
        }

        if (res_buf[0] != '\0') {
            if (config.msaa_enabled == FT_ENABLED
                    && (config.fb_enabled != FT_UNSUPPORTED
                    || config.ib_enabled != FT_UNSUPPORTED))
                osd_draw_stringf(110, y, "%s (%s)", res_buf, msaa_sm_buf);
            else
                osd_draw_stringf(110, y, "%s", res_buf);
        }
    }

    return TAI_CONTINUE(int, g_main.osd_hook_ref, pParam, sync);
}

const char *vg_main_get_self_filename() {
    const char *self = strrchr(g_main.sce_info.path, '/');
    return self ? self + 1 : g_main.sce_info.path;
}

vg_module_match_t vg_main_match_current_module(const char titleid[], const char self[], uint32_t nid, bool exact) {
    if (strncasecmp(titleid, g_main.titleid, TITLEID_LEN) && (exact || strncasecmp(titleid, TITLEID_ANY, TITLEID_LEN)))
        return MODULE_TITLE_MISMATCH;
    if (exact ? strcmp(self, vg_main_get_self_filename()) : self[0] && !strstr(g_main.sce_info.path, self))
        return MODULE_SELF_MISMATCH;
    if (nid != g_main.tai_info.module_nid && (exact || nid != NID_ANY))
        return MODULE_NID_MISMATCH;
    return MODULE_MATCH;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    // Get app titleid
    sceAppMgrAppParamGetString(0, 12, g_main.titleid, 16);

    // Exit if using VitaShell
    if (!strncmp(g_main.titleid, "VITASHELL", TITLEID_LEN)) {
        goto EXIT;
    }

    g_main.osd_hook = -1;
    g_main.inject_num = 0;
    for (int i = 0; i < MAX_INJECT_NUM; i++) {
        g_main.inject[i] = -1;
    }
    for (int i = 0; i < MAX_HOOK_NUM; i++) {
        g_main.hook[i] = -1;
    }

    // Get eboot.bin info
    g_main.tai_info.size = sizeof(tai_module_info_t);
    g_main.sce_info.size = sizeof(SceKernelModuleInfo);
    taiGetModuleInfo(TAI_MAIN_MODULE, &g_main.tai_info);
    sceKernelGetModuleInfo(g_main.tai_info.modid, &g_main.sce_info);

    // Create VitaGrafix folder (if doesn't exist)
    sceIoMkdir(VG_DIR, 0777);

    // Log basic info
    vg_log_printf("VitaGrafix " VG_VERSION "\n");
    vg_log_printf("=======================================\n");
    vg_log_printf("[MAIN] Title ID: %s\n", g_main.titleid);
    vg_log_printf("[MAIN] SELF: %s\n", g_main.sce_info.path);
    vg_log_printf("[MAIN] NID: 0x%X\n", g_main.tai_info.module_nid);
    vg_log_printf("=======================================\n");

    // Parse config.txt
    vg_io_status_t config_status = vg_config_parse();
    vg_io_status_t patch_status = {IO_OK, 0, 0};
    vg_config_t *config = vg_config_get();

    if (config->log_enabled == FT_ENABLED) {
        vg_log_prepare();
        vg_log_set_enabled(true);
    }

    if (config_status.code != IO_OK) {
        config->enabled = FT_ENABLED;
        config->osd_enabled = FT_ENABLED;
        vg_log_printf("[PATCH] Failed to parse config (line %d, pos %d): %s\n",
                    config_status.line, config_status.pos_line, vg_io_status_code_to_string(config_status.code));
    }

    // Exit now?
    if (config->enabled == FT_DISABLED)
        goto EXIT;

    // Skip parsing patchlist if there was an error in config
    if (config_status.code != IO_OK)
        goto EXIT_HOOK_OSD;

    // Parse patchlist & apply patches
    patch_status = vg_patch_parse_and_apply();
    if (patch_status.code != IO_OK) {
        config->osd_enabled = FT_ENABLED;
        vg_log_printf("[PATCH] Failed to parse patchlist (line %d, pos %d): %s\n",
                    patch_status.line, patch_status.pos_line, vg_io_status_code_to_string(patch_status.code));
        goto EXIT_HOOK_OSD;
    }

    // Exit if game is not supported / is self shell
    if (g_main.patch_match == MODULE_SELF_MISMATCH || g_main.patch_match == MODULE_TITLE_MISMATCH)
        goto EXIT;

EXIT_HOOK_OSD:
    // Hook sceDisplaySetFrameBuf for OSD
    if (config->osd_enabled) {
        g_main.osd_timer = 0;
        g_main.osd_hook = taiHookFunctionImport(
                    &g_main.osd_hook_ref,
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x7A410B64,
                    sceDisplaySetFrameBuf_patched);

        if (config_status.code != IO_OK || patch_status.code != IO_OK) {
            vg_log_read(g_osd_buffer, STRING_BUFFER_SIZE);
        }
    }

EXIT:
    vg_log_flush();
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    // Release OSD hook
    if (g_main.osd_hook >= 0) {
        taiHookRelease(g_main.osd_hook, g_main.osd_hook_ref);
    }

    // Release game patches
    for (uint32_t i = g_main.inject_num; i > 0; i--) {
        if (g_main.inject[i - 1] >= 0)
            taiInjectRelease(g_main.inject[i - 1]);
    }
    g_main.inject_num = 0;

    // Release game hooks, we need to loop the whole array since hooks are indexed by their id
    for (uint8_t i = MAX_HOOK_NUM; i > 0; i--) {
        if (g_main.hook[i - 1] >= 0)
            taiHookRelease(g_main.hook[i - 1], g_main.hook_ref[i - 1]);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
