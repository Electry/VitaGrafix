#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/ctype.h>

#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "config.h"
#include "tools.h"

#define MAX_INJECTS_NUM 2

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

	updateFramebuf(pParam);

	if (timer > 0) {
		int y = 5;
		drawStringF(5, y, "%s: %s", titleid, config.game_enabled ? "patched" : "disabled");

		if (config.game_enabled) {
			if (config.fb_res_enabled) {
				y += 20;
				drawStringF(5, y, "Framebuffer: %dx%d", config.fb_width, config.fb_height);
			}
			if (config.ib_res_enabled) {
				y += 20;
				drawStringF(5, y, "Internal: %dx%d", config.ib_width, config.ib_height);
			}
			if (config.fps_enabled) {
				y += 20;
				drawStringF(5, y, "FPS cap: %d", config.fps == FPS_30 ? 30 : 60);
			}
		}

		timer--;
	} else {
		// Release no longer used hook
		taiHookRelease(sceDisplaySetFrameBuf_hookid, sceDisplaySetFrameBuf_hookref);
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
	if (!config.enabled) {
		return SCE_KERNEL_START_SUCCESS;
	}

	if (strncmp(titleid, "PCSF00243", 9) == 0) { // Killzone Mercenary [EUR] [1.12] - 60fps
		if (config.fps_enabled) {
			uint32_t data32_vblank = config.fps == FPS_60 ? 0x1 : 0x2;

			// dword_819706A4  DCD 1 ; DATA XREF: seg000:8104F722
			injectData(info.modid, 0, 0x9706A4, &data32_vblank, sizeof(data32_vblank));
		}

		config.fb_res_enabled = 0;
		config.ib_res_enabled = 0;
		supported_game = 1;
	}
	else if (strncmp(titleid, "PCSB00245", 9) == 0 || // Persona 4 Golden [EUR] - 544p
			 strncmp(titleid, "PCSE00120", 9) == 0 || // Persona 4 Golden [USA] - 544p
			 strncmp(titleid, "PCSG00563", 9) == 0 || // Persona 4 Golden [JPN] - 544p
			 strncmp(titleid, "PCSH00021", 9) == 0) { // Persona 4 Golden [ASA] - 544p
		if (config.ib_res_enabled) {
			float float_width = (float)config.ib_width;
			float float_height = (float)config.ib_height;
			uint32_t offset_w, offset_h;

			if (strncmp(titleid, "PCSB00245", 9) == 0) {
				offset_w = 0xDBCFC; offset_h = 0xDBD00;
			}
			else if (strncmp(titleid, "PCSE00120", 9) == 0) {
				offset_w = 0xDBCEC; offset_h = 0xDBCF0;
			}
			else if (strncmp(titleid, "PCSG00563", 9) == 0) {
				offset_w = 0xDBD9C; offset_h = 0xDBDA0;
			}
			else if (strncmp(titleid, "PCSH00021", 9) == 0) {
				offset_w = 0xF1C50; offset_h = 0xF1C54;
			}

			injectData(info.modid, 1, offset_w, &float_width, sizeof(float_width));
			injectData(info.modid, 1, offset_h, &float_height, sizeof(float_height));
		}

		config.fb_res_enabled = 0;
		config.fps_enabled = 0;
		supported_game = 1;
	}

	if (supported_game && config.osd_enabled && config.game_osd_enabled) {
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

	if (supported_game)
		taiHookRelease(sceDisplaySetFrameBuf_hookid, sceDisplaySetFrameBuf_hookref);

	while (injects_num-- > 0) {
		taiInjectRelease(injects[injects_num]);
	}

	return SCE_KERNEL_STOP_SUCCESS;
}
