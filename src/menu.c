#include <vitasdk.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "main.h"
#include "menu.h"
#include "osd.h"

#define MENU_REPEAT_DELAY 300000
#define MENU_REPEAT_INTERVAL 33333
#define MENU_REPEAT_STEP 2
#define MENU_HEADER_ROWS 3
#define MENU_LEGEND "CROSS = toggle START = save"

static vg_menu_t g_menu = {0};

void vg_menu_init() {
    // Prepare rows that will be rendered based on feature availability
    g_menu.row_count = 0;
    g_menu.item_count = 0;

    if (vg_config_is_feature_supported(FEATURE_FB)) {
        g_menu.rows[g_menu.row_count++] = MENU_ROW_FB;
        g_menu.items[g_menu.item_count++] = MENU_ITEM_FB;
    }
    if (vg_config_is_feature_supported(FEATURE_IB)) {
        g_menu.rows[g_menu.row_count++] = MENU_ROW_IB;
        g_menu.items[g_menu.item_count++] = MENU_ITEM_IB_WIDTH;
        g_menu.items[g_menu.item_count++] = MENU_ITEM_IB_HEIGHT;
    }
    if (vg_config_is_feature_supported(FEATURE_FPS)) {
        g_menu.rows[g_menu.row_count++] = MENU_ROW_FPS;
        g_menu.items[g_menu.item_count++] = MENU_ITEM_FPS;
    }
    if (vg_config_is_feature_supported(FEATURE_MSAA)) {
        g_menu.rows[g_menu.row_count++] = MENU_ROW_MSAA;
        g_menu.items[g_menu.item_count++] = MENU_ITEM_MSAA;
    }

    if (g_menu.item_count > 0) {
        g_menu.selected_item = g_menu.items[0];
    } else {
        g_menu.selected_item = MENU_ITEM_INVALID;
    }

    g_menu.previous_buttons = 0;
    g_menu.repeat_button = 0;
    g_menu.repeat_until = 0;
    g_menu.selected_ib_index = 0;
    g_menu.open = false;
    g_menu.notice = MENU_NOTICE_NONE;
}

bool vg_menu_is_open() {
    return g_menu.open;
}

vg_menu_notice_t vg_menu_get_notice() {
    return g_menu.notice;
}

static int vg_menu_adjust_value(int value, int direction, int minimum, int maximum, int step) {
    int amount = (direction < 0 ? -direction : direction) * step;

    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    if (direction < 0) {
        return value - amount > minimum ? value - amount : minimum;
    }
    if (amount >= maximum - value) {
        return maximum;
    }
    return value + amount;
}

static void vg_menu_move_selected_item_value(int direction) {
    vg_config_t *config = vg_config_get();

    switch (g_menu.selected_item) {
        case MENU_ITEM_FB: {
            if (!vg_config_is_feature_enabled(FEATURE_FB)) {
                config->fb_enabled = FT_ENABLED;
            }

            int index = 0;

            for (int i = 0; i < FRAMEBUFFER_RESOLUTION_COUNT; i++) {
                if (config->fb.width == vg_config_framebuffer_resolutions[i].width
                        && config->fb.height == vg_config_framebuffer_resolutions[i].height) {
                    index = i;
                    break;
                }
            }

            index = vg_menu_adjust_value(index, -direction, 0,
                    FRAMEBUFFER_RESOLUTION_COUNT - 1, 1);

            config->fb = vg_config_framebuffer_resolutions[index];
            break;
        }

        case MENU_ITEM_FPS: {
            if (!vg_config_is_feature_enabled(FEATURE_FPS)) {
                config->fps_enabled = FT_ENABLED;
            }

            int index = config->fps == FPS_20 ? 0 : config->fps == FPS_30 ? 1 : 2;
            index = vg_menu_adjust_value(index, direction, 0, 2, 1);

            config->fps = index == 0 ? FPS_20 : index == 1 ? FPS_30 : FPS_60;
            break;
        }

        case MENU_ITEM_MSAA: {
            if (!vg_config_is_feature_enabled(FEATURE_MSAA)) {
                config->msaa_enabled = FT_ENABLED;
                break;
            }

            int index = config->msaa == MSAA_NONE ? 0 : config->msaa == MSAA_2X ? 1 : 2;
            index = vg_menu_adjust_value(index, direction, 0, 2, 1);

            config->msaa = index == 0 ? MSAA_NONE : index == 1 ? MSAA_2X : MSAA_4X;
            break;
        }

        case MENU_ITEM_IB_WIDTH: {
            if (!vg_config_is_feature_enabled(FEATURE_IB)) {
                config->ib_enabled = FT_ENABLED;
                break;
            }

            config->ib[g_menu.selected_ib_index].width = vg_menu_adjust_value(
                    config->ib[g_menu.selected_ib_index].width, direction, 4, 960, 4);
            break;
        }

        case MENU_ITEM_IB_HEIGHT: {
            if (!vg_config_is_feature_enabled(FEATURE_IB)) {
                config->ib_enabled = FT_ENABLED;
                break;
            }

            config->ib[g_menu.selected_ib_index].height = vg_menu_adjust_value(
                    config->ib[g_menu.selected_ib_index].height, direction, 4, 544, 4);
            break;
        }

        default:
            break;
    }
}

static void vg_menu_move_selected_item(int direction) {
    int selected = 0;
    for (int i = 0; i < g_menu.item_count; i++) {
        if (g_menu.items[i] == g_menu.selected_item) {
            selected = i;
            break;
        }
    }

    vg_menu_item_t target = g_menu.items[(selected + direction + g_menu.item_count) % g_menu.item_count];

    // treat IB as a single item if its not enabled
    if (!vg_config_is_feature_enabled(FEATURE_IB) && target == MENU_ITEM_IB_HEIGHT) {
        target = g_menu.items[(selected + direction + (direction > 0 ? 1 : -1) + g_menu.item_count) % g_menu.item_count];
    }

    g_menu.selected_item = target;
}

void vg_menu_check_input(SceCtrlData *ctrl) {
    vg_config_t *config = vg_config_get();

    uint32_t pressed = ctrl->buttons & ~g_menu.previous_buttons;
    g_menu.previous_buttons = ctrl->buttons;

    // SELECT + RIGHT TRIGGER: Toggle menu
    if (ctrl->buttons & SCE_CTRL_SELECT) {
        if (pressed & SCE_CTRL_RTRIGGER) {
            if (g_menu.item_count == 0)
                return;

            g_menu.selected_item = g_menu.items[0];

            g_menu.selected_ib_index = 0;
            g_menu.open = !g_menu.open;
            g_menu.notice = MENU_NOTICE_NONE;
            g_menu.repeat_button = 0;

            ctrl->buttons = 0;
            return;
        }
    }

    if (!g_menu.open) {
        return;
    }

    // D-PAD UP / DOWN: Move between menu items
    if (pressed & SCE_CTRL_UP) {
        vg_menu_move_selected_item(-1);
        goto EXIT;
    }
    if (pressed & SCE_CTRL_DOWN) {
        vg_menu_move_selected_item(1);
        goto EXIT;
    }

    // Holding D-PAD LEFT / RIGHT: Fast move on IB row
    uint32_t repeat_button = ctrl->buttons & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT);
    if ((g_menu.selected_item == MENU_ITEM_IB_WIDTH || g_menu.selected_item == MENU_ITEM_IB_HEIGHT) && repeat_button) {
        SceUInt32 now = sceKernelGetProcessTimeLow();
        int direction = ctrl->buttons & SCE_CTRL_LEFT ? -1 : 1;

        if (g_menu.repeat_button != repeat_button || (pressed & repeat_button)) {
            vg_menu_move_selected_item_value(direction);
            g_menu.repeat_button = repeat_button;
            g_menu.repeat_until = now + MENU_REPEAT_DELAY;
        } else if ((SceInt32)(now - g_menu.repeat_until) >= 0) {
            SceUInt32 intervals = (now - g_menu.repeat_until) / MENU_REPEAT_INTERVAL + 1;
            vg_menu_move_selected_item_value(direction * intervals * MENU_REPEAT_STEP);
            g_menu.repeat_until += intervals * MENU_REPEAT_INTERVAL;
        }

        goto EXIT;
    } else {
        g_menu.repeat_button = 0;
    }

    // D-PAD LEFT / RIGHT: Change item value
    if (pressed & SCE_CTRL_LEFT) {
        vg_menu_move_selected_item_value(-1);
        goto EXIT;
    }
    if (pressed & SCE_CTRL_RIGHT) {
        vg_menu_move_selected_item_value(1);
        goto EXIT;
    }

    // LEFT / RIGHT TRIGGER: Cycle IB res index on IB row
    if ((g_menu.selected_item == MENU_ITEM_IB_WIDTH || g_menu.selected_item == MENU_ITEM_IB_HEIGHT) && (pressed & SCE_CTRL_LTRIGGER)) {
        g_menu.selected_ib_index = (g_menu.selected_ib_index + MAX_RES_COUNT - 1) % MAX_RES_COUNT;
        goto EXIT;
    }
    if ((g_menu.selected_item == MENU_ITEM_IB_WIDTH || g_menu.selected_item == MENU_ITEM_IB_HEIGHT) && (pressed & SCE_CTRL_RTRIGGER)) {
        g_menu.selected_ib_index = (g_menu.selected_ib_index + 1) % MAX_RES_COUNT;
        goto EXIT;
    }

    // CROSS: Toggle selected item on/off
    if (pressed & SCE_CTRL_CROSS) {
        switch (g_menu.selected_item) {
            case MENU_ITEM_FB:
                config->fb_enabled = config->fb_enabled == FT_ENABLED ? FT_DISABLED : FT_ENABLED;
                break;

            case MENU_ITEM_IB_WIDTH:
            case MENU_ITEM_IB_HEIGHT:
                config->ib_enabled = config->ib_enabled == FT_ENABLED ? FT_DISABLED : FT_ENABLED;
                if (vg_config_is_feature_enabled(FEATURE_IB)) {
                    g_menu.selected_ib_index = 0;
                }
                break;

            case MENU_ITEM_FPS:
                config->fps_enabled = config->fps_enabled == FT_ENABLED ? FT_DISABLED : FT_ENABLED;
                break;

            case MENU_ITEM_MSAA:
                config->msaa_enabled = config->msaa_enabled == FT_ENABLED ? FT_DISABLED : FT_ENABLED;
                break;

            default:
                break;
        }

        goto EXIT;
    }

    // START: Save config
    if (pressed & SCE_CTRL_START) {
        g_menu.open = false;
        g_menu.repeat_button = 0;
        g_menu.notice = vg_config_save_current_title_override() ? MENU_NOTICE_SAVED : MENU_NOTICE_SAVE_FAILED;
        goto EXIT;
    }

EXIT:
    ctrl->buttons = 0;
}

static void vg_menu_draw_label(int x, int y, const char *label) {
    osd_set_text_color(255, 255, 255, 255);
    osd_draw_string(x, y, label);
}

static void vg_menu_draw_value(int x, int y, const char *value, bool selected) {
    osd_set_text_color(selected ? 255 : 230, selected ? 210 : 230, selected ? 64 : 230, 255);
    osd_draw_string(x, y, value);
}

void vg_menu_draw() {
    if (!g_menu.open) {
        return;
    }

    const vg_config_t *config = vg_config_get();

    int line_height = osd_get_text_height() + 4;

    int width = osd_get_text_width(MENU_LEGEND) + 28;
    int height = (g_menu.row_count + MENU_HEADER_ROWS + 1) * line_height + 20;

    int x = (960 - width) / 2;
    int y = (544 - height) / 2;

    int label_x = x + 14;
    int value_x = x + 14 + osd_get_text_width("IB (+99):") + 12; // account for the widest label
    osd_set_back_color(0, 0, 0, 255);
    osd_draw_rounded_rectangle(x, y, width, height, 7);

    osd_set_text_color(255, 255, 255, 255);
    osd_draw_string(x + 14, y + 10, "VitaGrafix " VG_VERSION);

    osd_set_text_color(180, 180, 180, 255);

    const char *self = vg_main_get_self_filename();
    int self_length = strlen(self);
    if (self_length > SELF_LEN_MAX) {
        self_length = SELF_LEN_MAX;
    }
    char title_info[64];
    do {
        snprintf(title_info, sizeof(title_info), "%s / %.*s", g_main.titleid, self_length, self);
        self_length--;
    } while (self_length >= 0 && osd_get_text_width(title_info) > width - 28);
    osd_draw_string(x + 14, y + 10 + line_height, title_info);

    char nid_info[32];
    snprintf(nid_info, sizeof(nid_info), "Fingerprint: 0x%08X", g_main.tai_info.module_nid);
    osd_draw_string(x + 14, y + 10 + 2 * line_height - 4, nid_info);

    for (int i = 0; i < g_menu.row_count; i++) {
        int row_y = y + 10 + (i + MENU_HEADER_ROWS) * line_height;
        int row_value_x = value_x;

        vg_menu_row_t row = g_menu.rows[i];

        if (row == MENU_ROW_FB) {
            vg_menu_draw_label(label_x, row_y, "FB:");

            bool selected = g_menu.selected_item == MENU_ITEM_FB;
            char value[24];
            if (vg_config_is_feature_enabled(FEATURE_FB)) {
                snprintf(value, sizeof(value), selected ? "< %dx%d >" : "%dx%d", config->fb.width, config->fb.height);
            } else {
                snprintf(value, sizeof(value), selected ? "< Off >" : "Off");
            }

            if (!selected) {
                row_value_x = osd_get_text_end_x(row_value_x, "< ");
            }
            vg_menu_draw_value(row_value_x, row_y, value, selected);

            continue;
        }

        if (row == MENU_ROW_IB) {
            char label[24] = "IB:";
            if (g_menu.selected_ib_index > 0) {
                snprintf(label, sizeof(label), "IB (+%d):", g_menu.selected_ib_index);
            }
            vg_menu_draw_label(label_x, row_y, label);

            bool row_selected = g_menu.selected_item == MENU_ITEM_IB_WIDTH || g_menu.selected_item == MENU_ITEM_IB_HEIGHT;

            if (!vg_config_is_feature_enabled(FEATURE_IB)) {
                if (!row_selected) {
                    row_value_x = osd_get_text_end_x(row_value_x, "< ");
                }
                vg_menu_draw_value(row_value_x, row_y, row_selected ? "< Off >" : "Off", row_selected);
            } else {
                char width[8], height[8];
                snprintf(width, sizeof(width), "%d", config->ib[g_menu.selected_ib_index].width);
                snprintf(height, sizeof(height), "%d", config->ib[g_menu.selected_ib_index].height);

                if (row_selected) {
                    vg_menu_draw_value(row_value_x, row_y, "< ", true);
                }
                row_value_x = osd_get_text_end_x(row_value_x, "< ");

                vg_menu_draw_value(row_value_x, row_y, width, g_menu.selected_item == MENU_ITEM_IB_WIDTH);
                row_value_x += osd_get_text_width(width);

                vg_menu_draw_value(row_value_x, row_y, "x", false);
                row_value_x += osd_get_text_width("x");

                vg_menu_draw_value(row_value_x, row_y, height, g_menu.selected_item == MENU_ITEM_IB_HEIGHT);
                row_value_x += osd_get_text_width(height);

                if (row_selected) {
                    vg_menu_draw_value(row_value_x, row_y, " >", true);
                }
            }

            continue;
        }

        if (row == MENU_ROW_FPS) {
            vg_menu_draw_label(label_x, row_y, "FPS:");

            bool selected = g_menu.selected_item == MENU_ITEM_FPS;
            const char *value = !vg_config_is_feature_enabled(FEATURE_FPS) ? "Off" :
                    config->fps == FPS_20 ? "20" : config->fps == FPS_30 ? "30" : "60";
            char display_value[16];
            snprintf(display_value, sizeof(display_value), selected ? "< %s >" : "%s", value);
            if (!selected) {
                row_value_x = osd_get_text_end_x(row_value_x, "< ");
            }
            vg_menu_draw_value(row_value_x, row_y, display_value, selected);

            continue;
        }

        if (row == MENU_ROW_MSAA) {
            vg_menu_draw_label(label_x, row_y, "MSAA:");

            bool selected = g_menu.selected_item == MENU_ITEM_MSAA;
            const char *value = !vg_config_is_feature_enabled(FEATURE_MSAA) ? "Off" :
                    config->msaa == MSAA_NONE ? "No MSAA" : config->msaa == MSAA_2X ? "2x" : "4x";
            char display_value[16];
            snprintf(display_value, sizeof(display_value), selected ? "< %s >" : "%s", value);
            if (!selected) {
                row_value_x = osd_get_text_end_x(row_value_x, "< ");
            }
            vg_menu_draw_value(row_value_x, row_y, display_value, selected);

            continue;
        }
    }

    osd_set_text_color(180, 180, 180, 255);
    osd_draw_string(x + 14, y + height - line_height - 4, MENU_LEGEND);
}
