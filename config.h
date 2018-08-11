#ifndef _CONFIG_H_
#define _CONFIG_H_

typedef enum {
	FEATURE_DISABLED = 0,
	FEATURE_ENABLED,
	FEATURE_UNSPECIFIED,
	FEATURE_UNSUPPORTED
} VG_FeatureState;

typedef enum {
	FPS_60,
	FPS_30
} VG_Fps;

typedef struct {
	VG_FeatureState enabled;
	VG_FeatureState osd_enabled;

	VG_FeatureState game_enabled;
	VG_FeatureState game_osd_enabled;

	// Framebuffer
	VG_FeatureState fb_res_enabled;
	uint16_t fb_width;
	uint16_t fb_height;

	// Internal buffer
	VG_FeatureState ib_res_enabled;
	uint16_t ib_width;
	uint16_t ib_height;

	// Framerate
	VG_FeatureState fps_enabled;
	VG_Fps fps;
} VG_Config;

VG_Config config_parse(const char *titleid);
void config_set_default(
		VG_FeatureState fb_res_enabled,
		VG_FeatureState ib_res_enabled,
		VG_FeatureState fps_enabled,
		VG_Config *config);
void config_set_unsupported(
		VG_FeatureState fb_res,
		VG_FeatureState ib_res,
		VG_FeatureState fps,
		VG_Config *config);

#endif
