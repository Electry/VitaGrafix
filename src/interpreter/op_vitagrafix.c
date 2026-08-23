#include <stdbool.h>

#include "interpreter.h"
#include "parser.h"
#include "op.h"

bool op_vg_config_fb_width(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->fb_width;
    return true;
}

bool op_vg_config_fb_height(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->fb_height;
    return true;
}

bool op_vg_config_ib_width(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->ib_width[0];
    return true;
}

bool op_vg_config_ib_height(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->ib_height[0];
    return true;
}

bool op_vg_config_ib_width_i(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    if ((out->type != DATA_TYPE_UNSIGNED
            && out->type != DATA_TYPE_SIGNED)
            || (out->type == DATA_TYPE_SIGNED && out->data.int32 < 0))
        return false;

    if (out->data.uint32 >= INTP_VG_MAX_RES_COUNT)
        return false;

    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->ib_width[out->data.uint32];
    return true;
}

bool op_vg_config_ib_height_i(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    if ((out->type != DATA_TYPE_UNSIGNED
            && out->type != DATA_TYPE_SIGNED)
            || (out->type == DATA_TYPE_SIGNED && out->data.int32 < 0))
        return false;

    if (out->data.uint32 >= INTP_VG_MAX_RES_COUNT)
        return false;

    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->ib_height[out->data.uint32];
    return true;
}

bool op_vg_config_vblank(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->vblank;
    return true;
}

bool op_vg_config_fps_limit(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->fps_limit;
    return true;
}

bool op_vg_config_msaa(value_t *out) {
    const intp_vg_context_t *context = intp_get_vg_context();
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    out->data.uint32 = context->msaa;
    return true;
}
