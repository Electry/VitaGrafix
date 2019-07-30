#ifndef _MAIN_H_
#define _MAIN_H_

#define SECOND             1000000
#define OSD_SHOW_DURATION  5 * SECOND

#define OSD_MSG_CONFIG_OPEN_FAILED   "Failed to open config file."
#define OSD_MSG_CONFIG_ERROR         "An error occured while reading config file."
#define OSD_MSG_PATCH_OPEN_FAILED    "Failed to open patch file."
#define OSD_MSG_PATCH_ERROR          "An error occured while reading patch file."
#define OSD_MSG_IOPLUS_HINT          "Do you have ioPlus installed?"
#define OSD_MSG_GAME_WRONG_VERSION   "Your game version is not supported :("

#define VG_VERSION         "v5.0.0"
#define VG_FOLDER          "ux0:data/VitaGrafix/"

#define STRING_BUFFER_SIZE 1024

#define MAX_INJECT_NUM 256
#define MAX_HOOK_NUM   4

#define TITLEID_LEN  9
#define TITLEID_ANY  "XXXXxxxxx"

#define SELF_LEN_MAX 32
#define SELF_ANY     ""
#define SELF_EBOOT   "eboot.bin"

#define NID_ANY      0

int isspace(int c);
int isdigit(int c);
int tolower(int c);

typedef enum {
    GAME_UNSUPPORTED,
    GAME_SELF_SHELL,
    GAME_WRONG_VERSION,
    GAME_SUPPORTED
} vg_game_support_t;

typedef struct {
    // OSD hook
    SceUID osd_hook;
    tai_hook_ref_t osd_hook_ref;
    SceUInt32 osd_timer;

    // title info
    char titleid[16];
    tai_module_info_t tai_info;
    SceKernelModuleInfo sce_info;

    // game support
    vg_game_support_t support;

    // eboot patches
    uint8_t inject_num;
    SceUID inject[MAX_INJECT_NUM];

    // eboot hooks
    uint8_t hook_num;
    SceUID hook[MAX_HOOK_NUM];
    tai_hook_ref_t hook_ref[MAX_HOOK_NUM];

    // user config
    vg_io_status_t config_status;
    vg_config_t config;

    // patchlist
    vg_io_status_t patch_status;
} vg_main_t;

extern vg_main_t g_main;

bool vg_main_is_game(const char titleid[], const char self[], uint32_t nid);

#endif
