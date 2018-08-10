#ifndef _CONFIG_H_
#define _CONFIG_H_

typedef enum {
	FPS_60,
	FPS_30
} VG_Fps;

typedef struct {
	uint8_t enabled;
	uint8_t osd_enabled;

	uint8_t game_enabled;
	uint8_t game_osd_enabled;

	// Framebuffer
	uint8_t fb_res_enabled;
	uint16_t fb_width;
	uint16_t fb_height;

	// Internal buffer
	uint8_t ib_res_enabled;
	uint16_t ib_width;
	uint16_t ib_height;

	// Framerate
	uint8_t fps_enabled;
	VG_Fps fps;
} VG_Config;

VG_Config config_parse(const char *titleid);

#endif
