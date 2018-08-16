#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/ctype.h>

#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "config.h"
#include "tools.h"
#include "games.h"
#include "main.h"

#define MAX_INJECTS_NUM 15

// sceDisplaySetFrameBuf hook
static SceUID sceDisplaySetFrameBuf_hookid;
static tai_hook_ref_t sceDisplaySetFrameBuf_hookref = {0};

// eboot patches
static uint8_t injects_num = 0;
static SceUID injects[MAX_INJECTS_NUM] = {0};

static char titleid[16];
static tai_module_info_t info = {0};

static uint8_t supported_game = 0;
static uint8_t timer = 150;

static VG_Config config;

void injectData(SceUID modid, int segidx, uint32_t offset, const void *data, size_t size) {
	injects[injects_num] = taiInjectData(modid, segidx, offset, data, size);
	injects_num++;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {

	if (timer > 0) {
		updateFramebuf(pParam);

		int y = 5;
		drawStringF(5, y, "%s: %s", titleid,
					config.game_enabled  == FT_ENABLED ? "patched" : "disabled");

		if (config_is_fb_enabled(&config)) {
			y += 20;
			drawStringF(5, y, "Framebuffer: %dx%d", config.fb_width, config.fb_height);
		}
		if (config_is_ib_enabled(&config)) {
			y += 20;
			drawStringF(5, y, "Internal: %dx%d", config.ib_width, config.ib_height);
		}
		if (config_is_fps_enabled(&config)) {
			y += 20;
			drawStringF(5, y, "FPS cap: %d", config.fps == FPS_30 ? 30 : 60);
		}

		timer--;
	} else {
		// Release no longer used hook
		int ret = TAI_CONTINUE(int, sceDisplaySetFrameBuf_hookref, pParam, sync);

		taiHookRelease(sceDisplaySetFrameBuf_hookid, sceDisplaySetFrameBuf_hookref);
		sceDisplaySetFrameBuf_hookid = -1;
		return ret;
	}

	return TAI_CONTINUE(int, sceDisplaySetFrameBuf_hookref, pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

	// Getting eboot.bin info
	info.size = sizeof(tai_module_info_t);
	taiGetModuleInfo(TAI_MAIN_MODULE, &info);

	// Getting app titleid
	sceAppMgrAppParamGetString(0, 12, titleid, 256);

	// Parse config
	config = config_parse(titleid);
	if (config.enabled == FT_DISABLED) {
		return SCE_KERNEL_START_SUCCESS;
	}

	// Patch the game
	supported_game = patch_game(titleid, &info, &config);

	// If game is unsupported, mark it as disabled
	if (!supported_game) {
		config.game_enabled = FT_DISABLED;
	}

	// If no features are enabled, mark game as disabled
	if (supported_game &&
			!config_is_fb_enabled(&config) &&
			!config_is_ib_enabled(&config) &&
			!config_is_fps_enabled(&config)) {
		config.game_enabled = FT_DISABLED;
	}

	// Hook sceDisplaySetFrameBuf for OSD
	if (supported_game && config_is_osd_enabled(&config)) {
		sceDisplaySetFrameBuf_hookid = taiHookFunctionImport(
				&sceDisplaySetFrameBuf_hookref,
				TAI_MAIN_MODULE,
				TAI_ANY_LIBRARY,
				0x7A410B64,
				sceDisplaySetFrameBuf_patched);
		timer = 150; // Show OSD for 150 frames
	}

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

	if (supported_game && config_is_osd_enabled(&config) && sceDisplaySetFrameBuf_hookid >= 0) {
		taiHookRelease(sceDisplaySetFrameBuf_hookid, sceDisplaySetFrameBuf_hookref);
	}

	while (injects_num-- > 0) {
		if (injects[injects_num] >= 0)
			taiInjectRelease(injects[injects_num]);
	}

	return SCE_KERNEL_STOP_SUCCESS;
}
