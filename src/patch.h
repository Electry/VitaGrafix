#ifndef _PATCH_H_
#define _PATCH_H_

#define PATCH_PATH        "ux0:data/VitaGrafix/patchlist.txt"
#define PATCH_MAX_LENGTH  32

typedef enum {
    PATCH_SECTION_NONE,
    PATCH_SECTION_GAME
} vg_patch_section_t;

typedef struct {
    const char *name;
    vg_feature_t type;
} vg_patch_feature_token_t;

void vg_patch_parse_and_apply();

#endif
