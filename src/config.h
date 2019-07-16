#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <stdbool.h>

#define MAX_RES_COUNT      16

#define CONFIG_PATH        "ux0:data/VitaGrafix/config.txt"

typedef enum {
    CONFIG_SECTION_NONE,
    CONFIG_SECTION_MAIN,
    CONFIG_SECTION_GAME
} vg_config_section_t;

typedef enum {
    FT_DISABLED = 0,
    FT_ENABLED,
    FT_UNSPECIFIED,
    FT_UNSUPPORTED
} vg_feature_state_t;

typedef enum {
    FEATURE_FB,
    FEATURE_IB,
    FEATURE_FPS,
    FEATURE_MSAA,
    FEATURE_INVALID
} vg_feature_t;

typedef enum {
    FPS_60,
    FPS_30,
    FPS_20
} vg_fps_t;

typedef enum {
    MSAA_4X,
    MSAA_2X,
    MSAA_NONE
} vg_msaa_t;

typedef struct {
    uint16_t width;
    uint16_t height;
} vg_res_t;

typedef struct {
    // Basic options
    vg_feature_state_t enabled;
    vg_feature_state_t osd_enabled;
    vg_feature_state_t log_enabled;

    // Framebuffer
    vg_feature_state_t fb_enabled;
    vg_res_t fb;

    // Internal buffer
    vg_feature_state_t ib_enabled;
    vg_res_t ib[MAX_RES_COUNT];
    uint8_t ib_count;

    // Framerate
    vg_feature_state_t fps_enabled;
    vg_fps_t fps;

    // MSAA
    vg_feature_state_t msaa_enabled;
    vg_msaa_t msaa;
} vg_config_t;

typedef enum {
    CONFIG_OPTION_FEATURE_STATE,
    CONFIG_OPTION_RESOLUTION,
    CONFIG_OPTION_FRAMERATE,
    CONFIG_OPTION_MSAA
} vg_config_parse_option_type_t;

typedef struct {
    const char *name;
    vg_config_parse_option_type_t type;
    vg_feature_state_t *ft_state;
    union {
        void *value;
        vg_res_t *res;
        vg_fps_t *fps;
        vg_msaa_t *msaa;
    };
    uint8_t *count;
} vg_config_parse_option_t;


void vg_config_parse();
bool vg_config_is_feature_enabled(vg_feature_t feature);
void vg_config_set_unsupported_features(vg_feature_state_t states[]);
void vg_config_set_supported_ib_count(uint8_t count);

//void vgConfigSetSupported(VG_FeatureState fb, VG_FeatureState ib, VG_FeatureState fps, VG_FeatureState msaa);
//void vgConfigSetSupportedIbCount(uint8_t count);
//uint8_t vgConfigIsFbEnabled();
//uint8_t vgConfigIsIbEnabled();
//uint8_t vgConfigIsFpsEnabled();
//uint8_t vgConfigIsMsaaEnabled();
//uint8_t vgConfigIsOsdEnabled();

#endif
