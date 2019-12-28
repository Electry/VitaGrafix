#include <stdbool.h>

#include "interpreter.h"
#include "parser.h"
#include "op.h"

#ifdef BUILD_VITAGRAFIX
#include <vitasdk.h>
#include <taihen.h>
#include "../io.h"
#include "../config.h"
#include "../log.h"
#include "../main.h"
#endif

bool op_vg_config_fb_width(value_t *out) {
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    out->data.uint32 = g_main.config.fb.width;
#else
    out->data.uint32 = 960;
#endif
    return true;
}

bool op_vg_config_fb_height(value_t *out) {
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    out->data.uint32 = g_main.config.fb.height;
#else
    out->data.uint32 = 544;
#endif
    return true;
}

bool op_vg_config_ib_width(value_t *out) {
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    out->data.uint32 = g_main.config.ib[0].width;
#else
    out->data.uint32 = 960;
#endif
    return true;
}

bool op_vg_config_ib_height(value_t *out) {
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    out->data.uint32 = g_main.config.ib[0].height;
#else
    out->data.uint32 = 544;
#endif
    return true;
}

bool op_vg_config_ib_width_i(value_t *out) {
    if ((out->type != DATA_TYPE_UNSIGNED
            && out->type != DATA_TYPE_SIGNED)
            || (out->type == DATA_TYPE_SIGNED && out->data.int32 < 0))
        return false;

#ifdef BUILD_VITAGRAFIX
    if (out->data.uint32 >= MAX_RES_COUNT)
        return false;
#endif

    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    out->data.uint32 = g_main.config.ib[out->data.uint32].width;
#else
    out->data.uint32 = 960;
#endif
    return true;
}

bool op_vg_config_ib_height_i(value_t *out) {
    if ((out->type != DATA_TYPE_UNSIGNED
            && out->type != DATA_TYPE_SIGNED)
            || (out->type == DATA_TYPE_SIGNED && out->data.int32 < 0))
        return false;

#ifdef BUILD_VITAGRAFIX
    if (out->data.uint32 >= MAX_RES_COUNT)
        return false;
#endif

    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    out->data.uint32 = g_main.config.ib[out->data.uint32].height;
#else
    out->data.uint32 = 544;
#endif
    return true;
}

bool op_vg_config_vblank(value_t *out) {
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    switch (g_main.config.fps) {
        case FPS_60: default: out->data.uint32 = 1; break;
        case FPS_30: out->data.uint32 = 2; break;
        case FPS_20: out->data.uint32 = 3; break;
    }
#else
    out->data.uint32 = 1;
#endif
    return true;
}

bool op_vg_config_msaa(value_t *out) {
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
#ifdef BUILD_VITAGRAFIX
    switch (g_main.config.msaa) {
        case MSAA_4X: default: out->data.uint32 = 2; break;
        case MSAA_2X: out->data.uint32 = 1; break;
        case MSAA_NONE: out->data.uint32 = 0; break;
    }
#else
    out->data.uint32 = 2;
#endif
    return true;
}
