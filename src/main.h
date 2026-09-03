#ifndef _MAIN_H_
#define _MAIN_H_

#include <taihen.h>

#define SECOND             1000000
#define OSD_SHOW_DURATION  5 * SECOND

#define OSD_ERROR_HEADER           "VitaGrafix " VG_VERSION ": Error"
#define OSD_MSG_CONFIG_OPEN_FAILED "Failed to open the config file."
#define OSD_MSG_CONFIG_ERROR       "An error occurred while reading the config file."
#define OSD_MSG_PATCH_OPEN_FAILED  "Failed to open the patch file."
#define OSD_MSG_PATCH_ERROR        "An error occurred while reading the patch file."
#define OSD_MSG_IOPLUS_HINT        "Do you have ioPlus installed?"
#define OSD_MSG_GAME_WRONG_VERSION "Patch is for another game version."
#define OSD_MSG_PATCHES_AVAILABLE  "Patches available."
#define OSD_MSG_CONFIG_SAVED       "Saved. Restart the game to apply."
#define OSD_MSG_CONFIG_SAVE_FAILED "Failed to save configuration."

#define VG_VERSION         "v6.0.0-dev"
#define VG_DIR             "ux0:data/VitaGrafix/"

#define STRING_BUFFER_SIZE 1024
#define INPUT_HOOK_NUM     8

#define MAX_INJECT_NUM 1024
#define MAX_HOOK_NUM   3

#define TITLEID_ANY  "XXXXxxxxx"

int isspace(int c);
int isdigit(int c);
int isalpha(int c);
int tolower(int c);

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

typedef enum {
    MODULE_TITLE_MISMATCH,
    MODULE_SELF_MISMATCH,
    MODULE_NID_MISMATCH,
    MODULE_MATCH
} vg_module_match_t;

typedef struct {
    // OSD hook
    SceUID osd_hook;
    tai_hook_ref_t osd_hook_ref;
    SceUInt32 osd_timer;

    // input hooks
    SceUID input_hook[INPUT_HOOK_NUM];
    tai_hook_ref_t input_hook_ref[INPUT_HOOK_NUM];

    // title info
    char titleid[16];
    tai_module_info_t tai_info;
    SceKernelModuleInfo sce_info;

    vg_module_match_t patch_match;

    // eboot patches
    uint32_t inject_num;
    SceUID inject[MAX_INJECT_NUM];

    // eboot hooks
    SceUID hook[MAX_HOOK_NUM];
    tai_hook_ref_t hook_ref[MAX_HOOK_NUM];

} vg_main_t;

extern vg_main_t g_main;

const char *vg_main_get_self_filename();
vg_module_match_t vg_main_match_current_module(const char titleid[], const char self[], uint32_t nid, bool require_exact_module);

#endif
