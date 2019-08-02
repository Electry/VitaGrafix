#ifndef _PATCH_H_
#define _PATCH_H_

#define PATCH_FOLDER      "ux0:data/VitaGrafix/patch/"
#define PATCH_LIST_PATH   "ux0:data/VitaGrafix/patchlist.txt"

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
