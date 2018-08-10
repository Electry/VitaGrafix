#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/ctype.h>
#include <vitasdk.h>
#include <taihen.h>
#include "display.h"

#define MAX_INJECTS_NUM 2

// sceDisplaySetFrameBuf hook
static SceUID sceDisplaySetFrameBuf_hookid;
static tai_hook_ref_t sceDisplaySetFrameBuf_hookref = {0};

// eboot patches
static uint8_t injects_num = 0;
static SceUID injects[MAX_INJECTS_NUM] = {0};

static char titleid[16];
static tai_module_info_t info = {0};

static uint8_t patched = 0;
static uint8_t timer = 150;

uint32_t data32_1 = 0x00000001;

uint32_t width_float = 0x44700000;
uint32_t height_float = 0x44080000;

void injectData(SceUID modid, int segidx, uint32_t offset, const void *data, size_t size) {
	injects[injects_num] = taiInjectData(modid, segidx, offset, data, size);
	injects_num++;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {

	updateFramebuf(pParam);

	if (timer > 0) {
		drawStringF(5, 5, "%s: %s", titleid, patched ? "patched" : "???");

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

	if (strncmp(titleid, "PCSF00243", 9) == 0) { // Killzone Mercenary [EUR] [1.12] - 60fps
		// dword_819706A4  DCD 1 ; DATA XREF: seg000:8104F722
		injectData(info.modid, 0, 0x9706A4, &data32_1, sizeof(data32_1));
		patched = 1;
	}
	else if (strncmp(titleid, "PCSB00245", 9) == 0) { // Persona 4 Golden [EUR] - 544p
		injectData(info.modid, 1, 0xDBCFC, &width_float, sizeof(width_float));
		injectData(info.modid, 1, 0xDBD00, &height_float, sizeof(height_float));
		patched = 1;
	}
	else if (strncmp(titleid, "PCSE00120", 9) == 0) { // Persona 4 Golden [USA] - 544p
		injectData(info.modid, 1, 0xDBCEC, &width_float, sizeof(width_float));
		injectData(info.modid, 1, 0xDBCF0, &height_float, sizeof(height_float));
		patched = 1;
	}
	else if (strncmp(titleid, "PCSG00563", 9) == 0) { // Persona 4 Golden [JPN] - 544p
		injectData(info.modid, 1, 0xDBD9C, &width_float, sizeof(width_float));
		injectData(info.modid, 1, 0xDBDA0, &height_float, sizeof(height_float));
		patched = 1;
	}
	else if (strncmp(titleid, "PCSH00021", 9) == 0) { // Persona 4 Golden [ASA] - 544p
		injectData(info.modid, 1, 0xF1C50, &width_float, sizeof(width_float));
		injectData(info.modid, 1, 0xF1C54, &height_float, sizeof(height_float));
		patched = 1;
	}

	if (patched) {
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

	taiHookRelease(sceDisplaySetFrameBuf_hookid, sceDisplaySetFrameBuf_hookref);

	while (injects_num-- > 0) {
		taiInjectRelease(injects[injects_num]);
	}

	return SCE_KERNEL_STOP_SUCCESS;
}
