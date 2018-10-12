#ifndef _PATCH_H_
#define _PATCH_H_

int sceDisplayWaitVblankStartMulti(unsigned int vcount);
int sceCtrlPeekBufferPositive2(int port, SceCtrlData *pad_data, int count);


void vgPatchGame();

#endif