#include <vitasdk.h>
#include <taihen.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "menu.h"
#include "patch.h"
#include "main.h"
#include "osd.h"

vg_main_t g_main = {0};

// string buffer
char g_osd_buffer[STRING_BUFFER_SIZE] = "";

#define DECL_FUNC_HOOK_INTERCEPT_CTRL(index, name, negative) \
    static int name##_patched(int port, SceCtrlData *ctrl, int count) { \
        int ret = TAI_CONTINUE(int, g_main.input_hook_ref[(index)], port, ctrl, count); \
        if (vg_menu_is_open()) { \
            for (int i = 0; i < ret; i++) \
                ctrl[i].buttons = (negative) ? ~0u : 0; \
        } \
        return ret; \
    }

DECL_FUNC_HOOK_INTERCEPT_CTRL(0, sceCtrlPeekBufferNegative, true)
DECL_FUNC_HOOK_INTERCEPT_CTRL(1, sceCtrlPeekBufferNegative2, true)
DECL_FUNC_HOOK_INTERCEPT_CTRL(2, sceCtrlPeekBufferPositive, false)
DECL_FUNC_HOOK_INTERCEPT_CTRL(3, sceCtrlPeekBufferPositive2, false)
DECL_FUNC_HOOK_INTERCEPT_CTRL(4, sceCtrlReadBufferNegative, true)
DECL_FUNC_HOOK_INTERCEPT_CTRL(5, sceCtrlReadBufferNegative2, true)
DECL_FUNC_HOOK_INTERCEPT_CTRL(6, sceCtrlReadBufferPositive, false)
DECL_FUNC_HOOK_INTERCEPT_CTRL(7, sceCtrlReadBufferPositive2, false)

static int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
    const vg_config_t config = *vg_config_get();
    const vg_io_status_t config_status = *vg_config_get_status();
    const vg_io_status_t patch_status = *vg_patch_get_status();

    // OSD not shown yet? Start the timer
    if (!g_main.osd_timer) {
        g_main.osd_timer = sceKernelGetProcessTimeLow();
    }

    osd_update_fb(pParam);

    if (g_main.patch_match == MODULE_MATCH) {
        bool menu_was_open = vg_menu_is_open();
        SceCtrlData ctrl;
        if (sceCtrlPeekBufferPositive(0, &ctrl, 1) > 0) {
            vg_menu_check_input(&ctrl);
        }

        // menu closed and notice is set, reset the OSD timer
        if (menu_was_open && !vg_menu_is_open() && vg_menu_get_notice() != MENU_NOTICE_NONE) {
            g_main.osd_timer = sceKernelGetProcessTimeLow();
        }

        if (vg_menu_is_open()) {
            vg_menu_draw();
            return TAI_CONTINUE(int, g_main.osd_hook_ref, pParam, sync);
        }
    }

    // OSD already shown, and all is good, return
    if (sceKernelGetProcessTimeLow() - g_main.osd_timer > OSD_SHOW_DURATION
            && config_status.code == IO_OK && patch_status.code == IO_OK) {
        return TAI_CONTINUE(int, g_main.osd_hook_ref, pParam, sync);
    }

    bool has_applied_patches = g_main.inject_num > 0;
    for (int i = 0; i < MAX_HOOK_NUM; i++) {
        if (g_main.hook[i] >= 0) {
            has_applied_patches = true;
            break;
        }
    }

    if (config_status.code != IO_OK || patch_status.code != IO_OK) {
        if (config_status.code == IO_ERROR_OPEN_FAILED) {
            osd_draw_header(OSD_ERROR_HEADER "\n" OSD_MSG_CONFIG_OPEN_FAILED "\n" OSD_MSG_IOPLUS_HINT);
        } else if (patch_status.code == IO_ERROR_OPEN_FAILED) {
            osd_draw_header(OSD_ERROR_HEADER "\n" OSD_MSG_PATCH_OPEN_FAILED "\n" OSD_MSG_IOPLUS_HINT);
        } else if (config_status.code != IO_OK) {
            osd_draw_header(OSD_ERROR_HEADER "\n" OSD_MSG_CONFIG_ERROR);
        } else if (patch_status.code != IO_OK) {
            osd_draw_header(OSD_ERROR_HEADER "\n" OSD_MSG_PATCH_ERROR);
        }

        osd_set_back_color(0, 0, 0, 255);
        if (config.log_enabled) {
            osd_draw_log(20, 110, pParam->height, g_osd_buffer);
        }
    } else if (g_main.patch_match == MODULE_NID_MISMATCH) {
        osd_draw_header(OSD_ERROR_HEADER "\n" OSD_MSG_GAME_WRONG_VERSION);
    } else if (vg_menu_get_notice() == MENU_NOTICE_SAVED) {
        osd_draw_header(OSD_MSG_CONFIG_SAVED);
    } else if (vg_menu_get_notice() == MENU_NOTICE_SAVE_FAILED) {
        osd_draw_header(OSD_MSG_CONFIG_SAVE_FAILED);
    } else if (g_main.patch_match == MODULE_MATCH && !has_applied_patches) {
        osd_draw_header(OSD_MSG_PATCHES_AVAILABLE);
    } else if (config.osd_enabled == FT_ENABLED) {
        char info[64] = "";

        if (config.fb_enabled == FT_ENABLED) {
            snprintf(info, sizeof(info), "%dx%d", config.fb.width, config.fb.height);
        } else if (config.ib_enabled == FT_ENABLED) {
            snprintf(info, sizeof(info), config.ib_count > 1 ? "%dx%d >> %dx%d" : "%dx%d",
                    config.ib[0].width, config.ib[0].height,
                    config.ib[config.ib_count - 1].width, config.ib[config.ib_count - 1].height);
        }
        if (config.fps_enabled == FT_ENABLED) {
            const int fps = config.fps == FPS_60 ? 60 : config.fps == FPS_30 ? 30 : 20;
            snprintf(info + strlen(info), sizeof(info) - strlen(info), "%s%d FPS", info[0] ? " / " : "", fps);
        }
        if (config.msaa_enabled == FT_ENABLED) {
            const char *msaa = config.msaa == MSAA_4X ? "4x MSAA" : config.msaa == MSAA_2X ? "2x MSAA" : "No MSAA";
            snprintf(info + strlen(info), sizeof(info) - strlen(info), "%s%s", info[0] ? " / " : "", msaa);
        }

        if (info[0]) {
            osd_draw_header(info);
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
    g_main.osd_hook = -1;
    g_main.inject_num = 0;
    for (int i = 0; i < MAX_INJECT_NUM; i++) {
        g_main.inject[i] = -1;
    }
    for (int i = 0; i < MAX_HOOK_NUM; i++) {
        g_main.hook[i] = -1;
    }
    for (int i = 0; i < INPUT_HOOK_NUM; i++) {
        g_main.input_hook[i] = -1;
    }

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
    sceIoMkdir(VG_DIR, 0777);

    // Log basic info
    vg_log_printf("VitaGrafix " VG_VERSION "\n");
    vg_log_printf("=======================================\n");
    vg_log_printf("[MAIN] Title ID: %s\n", g_main.titleid);
    vg_log_printf("[MAIN] SELF: %s\n", g_main.sce_info.path);
    vg_log_printf("[MAIN] NID: 0x%08X\n", g_main.tai_info.module_nid);
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

    if (config->osd_enabled == FT_ENABLED) {
        g_main.input_hook[0] = taiHookFunctionImport(&g_main.input_hook_ref[0], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0x104ED1A7, sceCtrlPeekBufferNegative_patched);
        g_main.input_hook[1] = taiHookFunctionImport(&g_main.input_hook_ref[1], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0x81A89660, sceCtrlPeekBufferNegative2_patched);
        g_main.input_hook[2] = taiHookFunctionImport(&g_main.input_hook_ref[2], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
        g_main.input_hook[3] = taiHookFunctionImport(&g_main.input_hook_ref[3], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0x15F81E8C, sceCtrlPeekBufferPositive2_patched);
        g_main.input_hook[4] = taiHookFunctionImport(&g_main.input_hook_ref[4], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0x15F96FB0, sceCtrlReadBufferNegative_patched);
        g_main.input_hook[5] = taiHookFunctionImport(&g_main.input_hook_ref[5], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0x27A0C5FB, sceCtrlReadBufferNegative2_patched);
        g_main.input_hook[6] = taiHookFunctionImport(&g_main.input_hook_ref[6], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0x67E7AB83, sceCtrlReadBufferPositive_patched);
        g_main.input_hook[7] = taiHookFunctionImport(&g_main.input_hook_ref[7], TAI_MAIN_MODULE,
                TAI_ANY_LIBRARY, 0xC4226A3E, sceCtrlReadBufferPositive2_patched);
        vg_menu_init();
    }

EXIT_HOOK_OSD:
    // Hook sceDisplaySetFrameBuf for OSD
    if (config->osd_enabled == FT_ENABLED) {
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

    // Release input hooks
    for (int i = 0; i < INPUT_HOOK_NUM; i++) {
        if (g_main.input_hook[i] >= 0) {
            taiHookRelease(g_main.input_hook[i], g_main.input_hook_ref[i]);
        }
    }

    // Release game patches
    for (uint32_t i = g_main.inject_num; i > 0; i--) {
        if (g_main.inject[i - 1] >= 0)
            taiInjectRelease(g_main.inject[i - 1]);
    }
    g_main.inject_num = 0;

    // Release game hooks: we need to loop the whole array since hooks are indexed by their id
    for (uint8_t i = MAX_HOOK_NUM; i > 0; i--) {
        if (g_main.hook[i - 1] >= 0)
            taiHookRelease(g_main.hook[i - 1], g_main.hook_ref[i - 1]);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
