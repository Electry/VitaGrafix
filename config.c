#include <psp2/types.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>

#include <libk/stdio.h>
#include <libk/stdlib.h>
#include <libk/string.h>
#include <kuio.h>

#include "config.h"

#define CONFIG_PATH "ux0:data/VitaGrafix/config.txt"

void config_parse_resolution(char *buffer, SceOff i, uint16_t *width, uint16_t *height) {
	*width = strtol(&buffer[i], NULL, 10);
	while (buffer[i++] != 'x') {}
	*height = strtol(&buffer[i], NULL, 10);
}

void config_parse_main(char *buffer, SceOff i, VG_Config *config) {
	if (strncmp(&buffer[i], "ENABLED=", 8) == 0) {
		i += 8;
		config->enabled = buffer[i] == '0' ? FT_DISABLED : FT_ENABLED;
	}
	else if (strncmp(&buffer[i], "OSD=", 4) == 0) {
		i += 4;
		config->osd_enabled = buffer[i] == '0' ? FT_DISABLED : FT_ENABLED;
	}
}

void config_parse_game(char *buffer, SceOff i, VG_Config *config) {
	if (strncmp(&buffer[i], "ENABLED=", 8) == 0) {
		i += 8;
		config->game_enabled = buffer[i] == '0' ? FT_DISABLED : FT_ENABLED;
	}
	else if (strncmp(&buffer[i], "OSD=", 4) == 0) {
		i += 4;
		config->game_osd_enabled = buffer[i] == '0' ? FT_DISABLED : FT_ENABLED;
	}
	else if (strncmp(&buffer[i], "FPS=", 4) == 0) {
		i += 4;
		config->fps_enabled = strncmp(&buffer[i], "OFF", 3) == 0 ? FT_DISABLED : FT_ENABLED;
		if (config->fps_enabled == FT_ENABLED)
			config->fps = strncmp(&buffer[i], "30", 2) == 0 ? FPS_30 : FPS_60;
	}
	else if (strncmp(&buffer[i], "FB=", 3) == 0) {
		i += 3;
		config->fb_res_enabled = strncmp(&buffer[i], "OFF", 3) == 0 ? FT_DISABLED : FT_ENABLED;
		if (config->fb_res_enabled == FT_ENABLED)
			config_parse_resolution(buffer, i, &(config->fb_width), &(config->fb_height));
	}
	else if (strncmp(&buffer[i], "IB=", 3) == 0) {
		i += 3;
		config->ib_res_enabled = strncmp(&buffer[i], "OFF", 3) == 0 ? FT_DISABLED : FT_ENABLED;
		if (config->ib_res_enabled == FT_ENABLED)
			config_parse_resolution(buffer, i, &(config->ib_width), &(config->ib_height));
	}
}

void config_set_unsupported(
		VG_FeatureState fb_res,
		VG_FeatureState ib_res,
		VG_FeatureState fps,
		VG_Config *config) {
	if (fb_res == FT_UNSUPPORTED)
		config->fb_res_enabled = fb_res;
	if (ib_res == FT_UNSUPPORTED)
		config->ib_res_enabled = ib_res;
	if (fps == FT_UNSUPPORTED)
		config->fps_enabled = fps;
}

void config_set_default(
		VG_FeatureState fb_res_enabled,
		VG_FeatureState ib_res_enabled,
		VG_FeatureState fps_enabled,
		VG_Config *config) {
	if (config->fb_res_enabled == FT_UNSPECIFIED)
		config->fb_res_enabled = fb_res_enabled;
	if (config->ib_res_enabled == FT_UNSPECIFIED)
		config->ib_res_enabled = ib_res_enabled;
	if (config->fps_enabled == FT_UNSPECIFIED)
		config->fps_enabled = fps_enabled;
}

void config_set_default_params(
		VG_FeatureState fb_res_enabled,
		uint16_t fb_width,
		uint16_t fb_height,
		VG_FeatureState ib_res_enabled,
		uint16_t ib_width,
		uint16_t ib_height,
		VG_FeatureState fps_enabled,
		VG_Fps fps,
		VG_Config *config) {
	if (config->fb_res_enabled == FT_UNSPECIFIED) {
		config->fb_res_enabled = fb_res_enabled;
		config->fb_width = fb_width;
		config->fb_height = fb_height;
	}
	if (config->ib_res_enabled == FT_UNSPECIFIED) {
		config->ib_res_enabled = ib_res_enabled;
		config->ib_width = ib_width;
		config->ib_height = ib_height;
	}
	if (config->fps_enabled == FT_UNSPECIFIED) {
		config->fps_enabled = fps_enabled;
		config->fps = fps;
	}
}

uint8_t config_is_fb_enabled(VG_Config *config) {
	return config->enabled == FT_ENABLED &&
			config->game_enabled == FT_ENABLED &&
			config->fb_res_enabled == FT_ENABLED;
}
uint8_t config_is_ib_enabled(VG_Config *config) {
	return config->enabled == FT_ENABLED &&
			config->game_enabled == FT_ENABLED &&
			config->ib_res_enabled == FT_ENABLED;
}
uint8_t config_is_fps_enabled(VG_Config *config) {
	return config->enabled == FT_ENABLED &&
			config->game_enabled == FT_ENABLED &&
			config->fps_enabled == FT_ENABLED;
}
uint8_t config_is_osd_enabled(VG_Config *config) {
	return config->enabled == FT_ENABLED &&
			config->osd_enabled == FT_ENABLED &&
			config->game_osd_enabled == FT_ENABLED;
}

VG_Config config_parse(const char *titleid) {
	VG_Config config = {
		FT_ENABLED, FT_ENABLED,
		FT_ENABLED, FT_ENABLED,
		FT_UNSPECIFIED, 960, 544,
		FT_UNSPECIFIED, 960, 544,
		FT_UNSPECIFIED, FPS_60
	};

	SceUID fd = -1;
	SceOff fileSize = 0;

	kuIoOpen(CONFIG_PATH, SCE_O_RDONLY, &fd);
	if (fd < 0) {
		return config;
	}

	kuIoLseek(fd, 0, SCE_SEEK_END);
	kuIoTell(fd, &fileSize);
	if (fileSize <= 0) {
		return config;
	}

	kuIoLseek(fd, 0, SCE_SEEK_SET);

	char buffer[fileSize];
	memset(buffer, 0, fileSize);
	kuIoRead(fd, buffer, fileSize);

	char section = 0; // 1=main, 2=titleid

	for (SceOff i = 0; i < fileSize; i++) {
		// New section
		if (buffer[i] == '[') {
			if (strncmp(&buffer[i], "[MAIN]", 6) == 0) {
				i += 7;
				section = 1;
			}
			else if (strncmp(&buffer[i+1], titleid, 9) == 0) {
				i += 12;
				section = 2;
			}
			else {
				section = 0;
			}
		}

		// [MAIN]
		if (section == 1) {
			config_parse_main(buffer, i, &config);
		}
		// [titleid]
		else if (section == 2) {
			config_parse_game(buffer, i, &config);
		}

		while (i < fileSize && buffer[i] != '\n') { i++; }
	}

	kuIoClose(fd);
	return config;
}
