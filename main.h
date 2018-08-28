#ifndef _MAIN_H_
#define _MAIN_H_

void injectData(SceUID modid, int segidx, uint32_t offset, const void *data, size_t size);
void hookFunction(SceUID modid, int segidx, uint32_t offset, int thumb, const void *func);
void hookFunctionImport(uint32_t nid, const void *func);

extern tai_hook_ref_t hooks_ref[];

#endif