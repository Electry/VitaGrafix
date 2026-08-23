#ifndef _PATCH_H_
#define _PATCH_H_

#include "io.h"
#include "config.h"

#define PATCH_DIR       "ux0:data/VitaGrafix/patch/"
#define PATCH_LIST_PATH "ux0:data/VitaGrafix/patchlist.txt"

#define PATCH_ALTE_PATH_LEN 32

typedef enum {
    PATCH_SECTION_NONE,
    PATCH_SECTION_GAME
} vg_patch_section_t;

typedef struct {
    const char *name;
    vg_feature_t type;
} vg_patch_feature_token_t;

vg_io_status_t vg_patch_parse_and_apply();
const vg_io_status_t *vg_patch_get_status();

#endif
