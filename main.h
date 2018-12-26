#ifndef _MAIN_H_
#define _MAIN_H_

#define SECOND             1000000
#define OSD_SHOW_DURATION  20 * SECOND

#define MSG_CONFIG_BAD          "Config file is either corrupt or contains invalid options!"
#define MSG_PATCH_BAD           "Patch file is invalid or contains syntax error! Check log.txt"
#define MSG_CONFIG_OPEN_FAILED  "Failed to open config file. Do you have ioPlus installed?"
#define MSG_PATCH_OPEN_FAILED   "Failed to open patch file. Do you have ioPlus installed?"
#define MSG_GAME_WRONG_VERSION  "Your game version is not supported! Update/refer to README.md"

#define VG_FOLDER          "ux0:data/VitaGrafix/"


#define MAX_INJECT_NUM 40
#define MAX_HOOK_NUM   10

#define TITLEID_LEN  9
#define TITLEID_ANY  "XXXXxxxxx"

#define SELF_LEN_MAX 32
#define SELF_ANY     ""
#define SELF_EBOOT   "eboot.bin"

#define NID_ANY     0


int snprintf(char * s, size_t n, const char * format, ...);
long int strtol(const char *str, char **endptr, int base);
unsigned long int strtoul(const char* str, char** endptr, int base);
int isspace(int c);


typedef enum {
    GAME_UNSUPPORTED,
    GAME_WRONG_VERSION,
    GAME_SELF_SHELL,
    GAME_SUPPORTED
} VG_GameSupport;

typedef struct {
    // OSD hook
    SceUID sceDisplaySetFrameBuf_hookid;
    tai_hook_ref_t sceDisplaySetFrameBuf_hookref;
    SceUInt32 timer;

    // title info
    char titleid[16];
    tai_module_info_t info;
    SceKernelModuleInfo sceInfo;

    // game support
    VG_GameSupport support;

    // eboot patches
    uint8_t inject_num;
    SceUID inject[MAX_INJECT_NUM];

    // eboot hooks
    uint8_t hook_num;
    SceUID hook[MAX_HOOK_NUM];
    tai_hook_ref_t hook_ref[MAX_HOOK_NUM];

    // user config
    VG_IoParseState config_state;
    VG_Config config;

    // patchlist
    VG_IoParseState patch_state;
} VG_Main;

extern VG_Main g_main;

void vgInjectData(int segidx, uint32_t offset, const void *data, size_t size);
void vgHookFunction(int segidx, uint32_t offset, int thumb, const void *func);
void vgHookFunctionImport(uint32_t nid, const void *func);

#endif