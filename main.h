#ifndef _MAIN_H_
#define _MAIN_H_

#define MAX_INJECT_NUM 30
#define MAX_HOOK_NUM   2

#define TITLEID_LEN 9
#define TITLEID_ANY "XXXXxxxxx"

#define SELF_ANY    ""
#define SELF_EBOOT  "eboot.bin"

#define NID_ANY     0

#define MSG_CONFIG_BAD          "Config file is either corrupt or contains invalid options!"
#define MSG_CONFIG_OPEN_FAILED  "Failed to open config file. Do you have ioPlus installed?"
#define MSG_GAME_WRONG_VERSION  "Your game version is not supported! Update/refer to README.md"

#define OSD_SHOW_DURATION  1000000 * 5

#define VG_FOLDER          "ux0:data/VitaGrafix/"


typedef enum {
    GAME_UNSUPPORTED,
    GAME_WRONG_VERSION,
    GAME_SELF_SHELL,
    GAME_SUPPORTED
} VG_GameSupport;

typedef struct {
    // sceDisplaySetFrameBuf hook
    SceUID sceDisplaySetFrameBuf_hookid;
    tai_hook_ref_t sceDisplaySetFrameBuf_hookref;

    // eboot patches
    uint8_t inject_num;
    SceUID inject[MAX_INJECT_NUM];

    // eboot hooks
    uint8_t hook_num;
    SceUID hook[MAX_HOOK_NUM];
    tai_hook_ref_t hook_ref[MAX_HOOK_NUM];

    // offsets
    uint32_t offset[MAX_HOOK_NUM + MAX_INJECT_NUM];

    char titleid[16];
    tai_module_info_t info;
    SceKernelModuleInfo sceInfo;

    VG_GameSupport support;
    SceUInt32 timer;

    VG_ConfigState config_state;
    VG_Config config;
} VG_Main;


extern VG_Main g_main;

void vgInjectData(int segidx, uint32_t offset, const void *data, size_t size);
void vgHookFunction(int segidx, uint32_t offset, int thumb, const void *func);
void vgHookFunctionImport(uint32_t nid, const void *func);

int snprintf(char *s, size_t n, const char *format, ...);
int vsnprintf(char *s, size_t n, const char *format, va_list arg);
long int strtol(const char *str, char **endptr, int base);

#endif
