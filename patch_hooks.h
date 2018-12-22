#ifndef _PATCH_HOOKS_H_
#define _PATCH_HOOKS_H_

int sceCtrlPeekBufferPositive2(int port, SceCtrlData *pad_data, int count);

VG_IoParseState vgPatchParseHook(
            const char chunk[], int pos, int end,
            uint32_t *importNid, void **hookPtr, uint8_t *shallHook);
#endif