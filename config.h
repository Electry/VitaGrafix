#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_RES_COUNT      16

#define CONFIG_PATH        "ux0:data/VitaGrafix/config.txt"

typedef enum {
    CONFIG_NONE,
    CONFIG_MAIN,
    CONFIG_GAME
} VG_ConfigSection;

typedef enum {
    FT_DISABLED = 0,
    FT_ENABLED,
    FT_UNSPECIFIED,
    FT_UNSUPPORTED
} VG_FeatureState;

typedef enum {
    FPS_60,
    FPS_30
} VG_Fps;

typedef enum {
    MSAA_4X,
    MSAA_2X,
    MSAA_NONE
} VG_Msaa;

typedef struct {
    uint16_t width;
    uint16_t height;
} VG_Resolution;

typedef struct {
    // [MAIN] options
    VG_FeatureState enabled;
    VG_FeatureState osd_enabled;

    // [TITLEID] options
    VG_FeatureState game_enabled;
    VG_FeatureState game_osd_enabled;

    // Framebuffer
    VG_FeatureState fb_enabled;
    VG_Resolution fb;

    // Internal buffer
    VG_FeatureState ib_enabled;
    VG_Resolution ib[MAX_RES_COUNT];
    uint8_t ib_count;

    // Framerate
    VG_FeatureState fps_enabled;
    VG_Fps fps;

    // MSAA
    VG_FeatureState msaa_enabled;
    VG_Msaa msaa;
} VG_Config;


void vgConfigParse();
void vgConfigSetSupported(VG_FeatureState fb, VG_FeatureState ib, VG_FeatureState fps, VG_FeatureState msaa);
void vgConfigSetSupportedIbCount(uint8_t count);
uint8_t vgConfigIsFbEnabled();
uint8_t vgConfigIsIbEnabled();
uint8_t vgConfigIsFpsEnabled();
uint8_t vgConfigIsMsaaEnabled();
uint8_t vgConfigIsOsdEnabled();

#endif
