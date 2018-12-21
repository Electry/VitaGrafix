#ifndef _PATCH_H_
#define _PATCH_H_

#define PATCH_PATH        "ux0:data/VitaGrafix/patchlist.txt"
#define PATCH_MAX_LENGTH  32

typedef enum {
    PATCH_NONE,
    PATCH_GAME
} VG_PatchSection;

typedef enum {
    PATCH_TYPE_NONE,
    PATCH_TYPE_OFF,
    PATCH_TYPE_FB,
    PATCH_TYPE_IB,
    PATCH_TYPE_FPS
} VG_PatchType;

void vgPatchParse();

#endif