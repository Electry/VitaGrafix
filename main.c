#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/ctype.h>
#include <vitasdk.h>
#include <taihen.h>

#define MAX_INJECTS_NUM 2

static uint8_t injects_num = 0;
static SceUID injects[MAX_INJECTS_NUM] = {0};

static char titleid[16];
tai_module_info_t info = {0};

uint32_t data32_1 = 0x00000001;

uint32_t width_float = 0x44700000;
uint32_t height_float = 0x44080000;

void injectData(SceUID modid, int segidx, uint32_t offset, const void *data, size_t size) {
	injects[injects_num] = taiInjectData(modid, segidx, offset, data, size);
	injects_num++;
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
	}
	else if (strncmp(titleid, "PCSB00245", 9) == 0) { // Persona 4 Golden [EUR] - 544p
		injectData(info.modid, 1, 0xDBCFC, &width_float, sizeof(width_float));
		injectData(info.modid, 1, 0xDBD00, &height_float, sizeof(height_float));
	}

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

	while (injects_num-- > 0) {
		taiInjectRelease(injects[injects_num]);
	}

	return SCE_KERNEL_STOP_SUCCESS;
}
