#ifndef _PATCH_HOOKS_H_
#define _PATCH_HOOKS_H_

int sceCtrlPeekBufferPositive2(int port, SceCtrlData *pad_data, int count);

vg_io_status_t vg_hook_parse_patch(const char line[]);

#endif
