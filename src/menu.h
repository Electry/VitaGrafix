#ifndef _MENU_H_
#define _MENU_H_

#include <stdbool.h>
#include <vitasdk.h>

typedef enum {
    MENU_ROW_FB,
    MENU_ROW_IB,
    MENU_ROW_FPS,
    MENU_ROW_MSAA,
    MENU_ROW_INVALID
} vg_menu_row_t;

typedef enum {
    MENU_ITEM_FB,
    MENU_ITEM_IB_WIDTH,
    MENU_ITEM_IB_HEIGHT,
    MENU_ITEM_FPS,
    MENU_ITEM_MSAA,
    MENU_ITEM_INVALID
} vg_menu_item_t;

typedef enum {
    MENU_NOTICE_NONE,
    MENU_NOTICE_SAVED,
    MENU_NOTICE_SAVE_FAILED
} vg_menu_notice_t;

typedef struct {
    bool open;

    uint32_t previous_buttons;
    uint32_t repeat_button;
    SceUInt32 repeat_until;

    vg_menu_row_t rows[MENU_ROW_INVALID];
    int row_count;

    vg_menu_item_t items[MENU_ITEM_INVALID];
    int item_count;

    vg_menu_item_t selected_item;
    uint8_t selected_ib_index;

    vg_menu_notice_t notice;
} vg_menu_t;

void vg_menu_init();
bool vg_menu_is_open();
vg_menu_notice_t vg_menu_get_notice();
void vg_menu_check_input(SceCtrlData *ctrl);
void vg_menu_draw();

#endif
