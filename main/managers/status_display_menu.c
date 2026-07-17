#include "sdkconfig.h"

#ifdef CONFIG_WITH_STATUS_DISPLAY

#include "managers/status_display_menu.h"
#include <string.h>

#define MENU_TOP_BAR_H 14
#define MENU_ITEM_H 14
#define MENU_ICON_GAP 4
#define MENU_TEXT_X (MENU_ICON_SIZE + MENU_ICON_GAP + 2)
#define MENU_VISIBLE_ITEMS 3
#define MENU_SCROLL_BAR_W 3

static const char *TAG = "StatusMenu";

static bool s_menu_active;
static int s_selected_index;
static int s_scroll_offset;
static int s_current_level;
static int s_level_stack[MENU_MAX_SUBMENUS];

// 8x8 icons stored as bytes, MSB-first, row by row
static const uint8_t icon_wifi[] = {
    0x00, 0x00, 0x00, 0x42, 0x24, 0x18, 0x00, 0x00
};

static const uint8_t icon_ble[] = {
    0x20, 0x30, 0x28, 0xFE, 0x28, 0x30, 0x20, 0x00
};

static const uint8_t icon_nfc[] = {
    0x3C, 0x42, 0x5A, 0x52, 0x5A, 0x42, 0x3C, 0x00
};

static const uint8_t icon_back[] = {
    0x00, 0x10, 0x38, 0x7E, 0x38, 0x10, 0x00, 0x00
};

static const uint8_t icon_scan[] = {
    0x38, 0x44, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00
};

static const uint8_t icon_deauth[] = {
    0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00, 0x00
};

static const uint8_t icon_beacon[] = {
    0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08, 0x00
};

static const uint8_t icon_stop[] = {
    0x00, 0x7E, 0x7E, 0x7E, 0x7E, 0x00, 0x00, 0x00
};

static const uint8_t icon_info[] = {
    0x3C, 0x42, 0x5A, 0x52, 0x5A, 0x42, 0x3C, 0x00
};

static const uint8_t icon_settings[] = {
    0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00, 0x00
};

static const uint8_t icon_sweep[] = {
    0x01, 0x02, 0x84, 0x48, 0x24, 0x12, 0x09, 0x06
};

static const uint8_t icon_spam[] = {
    0x7E, 0x42, 0x5A, 0x5A, 0x5A, 0x42, 0x7E, 0x00
};

static const uint8_t icon_station[] = {
    0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x3C, 0x00
};

static const uint8_t icon_raw[] = {
    0x00, 0x66, 0x24, 0x24, 0x24, 0x66, 0x00, 0x00
};

static const menu_item_t wifi_items[] = {
    {"Back",         icon_back,     MENU_ACTION_BACK},
    {"Scan AP",      icon_scan,     MENU_ACTION_WIFI_SCAN},
    {"Station Scan", icon_station,  MENU_ACTION_WIFI_STATION_SCAN},
    {"Deauth",       icon_deauth,   MENU_ACTION_WIFI_DEAUTH},
    {"Beacon Spam",  icon_beacon,   MENU_ACTION_WIFI_BEACON},
    {"Rickroll",     icon_beacon,   MENU_ACTION_WIFI_BEACON_RICKROLL},
    {"EAPOL Logoff", icon_deauth,   MENU_ACTION_WIFI_EAPOL},
    {"Karma",        icon_beacon,   MENU_ACTION_WIFI_KARMA},
};

static const menu_item_t ble_items[] = {
    {"Back",          icon_back,     MENU_ACTION_BACK},
    {"GATT Scan",     icon_scan,     MENU_ACTION_BLE_GATT_SCAN},
    {"Flipper Scan",  icon_scan,     MENU_ACTION_BLE_FLIPPER_SCAN},
    {"AirTag Scan",   icon_scan,     MENU_ACTION_BLE_AIRTAG_SCAN},
    {"Advertisers",   icon_scan,     MENU_ACTION_BLE_ADVERTISER_SCAN},
    {"Raw Packets",   icon_raw,      MENU_ACTION_BLE_RAW_SCAN},
    {"Spam Apple",    icon_spam,     MENU_ACTION_BLE_SPAM_APPLE},
    {"Spam Samsung",  icon_spam,     MENU_ACTION_BLE_SPAM_SAMSUNG},
    {"Spam Google",   icon_spam,     MENU_ACTION_BLE_SPAM_GOOGLE},
};

static const menu_item_t nfc_items[] = {
    {"Back",    icon_back,     MENU_ACTION_BACK},
    {"Scan",    icon_scan,     MENU_ACTION_NFC_SCAN},
    {"Emulate", icon_nfc,      MENU_ACTION_NFC_EMULATE},
};

static const menu_item_t main_items[] = {
    {"WiFi",      icon_wifi,      MENU_ACTION_NONE},
    {"BLE",       icon_ble,       MENU_ACTION_NONE},
    {"NFC",       icon_nfc,       MENU_ACTION_NONE},
    {"Sweep All", icon_sweep,     MENU_ACTION_SWEEP},
    {"Stop All",  icon_stop,      MENU_ACTION_STOP_ALL},
    {"Settings",  icon_settings,  MENU_ACTION_SETTINGS},
    {"Info",      icon_info,      MENU_ACTION_INFO},
};

static const menu_level_t levels[] = {
    {"GhostESP", main_items, sizeof(main_items) / sizeof(main_items[0])},
    {"WiFi",     wifi_items, sizeof(wifi_items) / sizeof(wifi_items[0])},
    {"BLE",      ble_items,  sizeof(ble_items) / sizeof(ble_items[0])},
    {"NFC",      nfc_items,  sizeof(nfc_items) / sizeof(nfc_items[0])},
};

#define NUM_LEVELS (sizeof(levels) / sizeof(levels[0]))

static void ensure_selection_visible(void) {
    const menu_level_t *lvl = &levels[s_current_level];
    if (s_selected_index < s_scroll_offset) {
        s_scroll_offset = s_selected_index;
    } else if (s_selected_index >= s_scroll_offset + MENU_VISIBLE_ITEMS) {
        s_scroll_offset = s_selected_index - MENU_VISIBLE_ITEMS + 1;
    }
    if (s_scroll_offset < 0) s_scroll_offset = 0;
    int max_scroll = lvl->count - MENU_VISIBLE_ITEMS;
    if (max_scroll < 0) max_scroll = 0;
    if (s_scroll_offset > max_scroll) s_scroll_offset = max_scroll;
}

static void draw_icon(const menu_gfx_t *gfx, int x, int y, const uint8_t *icon) {
    if (!icon) return;
    for (int row = 0; row < 8; ++row) {
        uint8_t bits = icon[row];
        for (int col = 0; col < 8; ++col) {
            bool on = (bits >> (7 - col)) & 0x01;
            if (on) {
                for (int sy = 0; sy < gfx->scale_y; ++sy) {
                    gfx->plot_pixel(gfx->user, x + col, y + row * gfx->scale_y + sy, true);
                }
            }
        }
    }
}

static void draw_inverted_rect(const menu_gfx_t *gfx, int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; ++y) {
        for (int x = x0; x < x0 + w; ++x) {
            gfx->plot_pixel(gfx->user, x, y, true);
        }
    }
}

static void draw_hline(const menu_gfx_t *gfx, int x, int y, int w) {
    for (int i = 0; i < w; ++i) {
        gfx->plot_pixel(gfx->user, x + i, y, true);
    }
}

void status_menu_init(void) {
    s_menu_active = false;
    s_selected_index = 0;
    s_scroll_offset = 0;
    s_current_level = 0;
    memset(s_level_stack, 0, sizeof(s_level_stack));
}

bool status_menu_is_active(void) {
    return s_menu_active;
}

void status_menu_activate(void) {
    s_menu_active = true;
    s_current_level = 0;
    s_selected_index = 0;
    s_scroll_offset = 0;
    memset(s_level_stack, 0, sizeof(s_level_stack));
}

void status_menu_deactivate(void) {
    s_menu_active = false;
}

void status_menu_handle_press(void) {
    if (!s_menu_active) return;
    const menu_level_t *lvl = &levels[s_current_level];
    s_selected_index++;
    if (s_selected_index >= lvl->count) {
        s_selected_index = 0;
        s_scroll_offset = 0;
    }
    ensure_selection_visible();
}

void status_menu_handle_long_press(void) {
    if (!s_menu_active) return;
    const menu_level_t *lvl = &levels[s_current_level];
    if (s_selected_index < 0 || s_selected_index >= lvl->count) return;
    const menu_item_t *item = &lvl->items[s_selected_index];

    if (item->action == MENU_ACTION_BACK) {
        if (s_current_level > 0) {
            s_current_level = s_level_stack[--s_current_level];
            s_selected_index = 0;
            s_scroll_offset = 0;
            ensure_selection_visible();
        } else {
            status_menu_deactivate();
        }
        return;
    }

    if (item->action == MENU_ACTION_NONE) {
        if (s_current_level < MENU_MAX_SUBMENUS - 1) {
            int target = s_selected_index + 1;
            if (target < (int)NUM_LEVELS) {
                s_level_stack[s_current_level] = s_selected_index;
                s_current_level = target;
                s_selected_index = 0;
                s_scroll_offset = 0;
                ensure_selection_visible();
            }
        }
        return;
    }
}

void status_menu_render(const menu_gfx_t *gfx) {
    if (!gfx || !s_menu_active) return;
    const menu_level_t *lvl = &levels[s_current_level];
    if (!lvl) return;

    int w = gfx->width;
    int h = gfx->height;
    int char_h = 7 * gfx->scale_y;
    int char_w = gfx->font_char_width + 1;

    // draw top bar background
    draw_inverted_rect(gfx, 0, 0, w, MENU_TOP_BAR_H);

    // draw title centered in top bar
    int title_len = (int)strlen(lvl->title);
    int title_x = (w - title_len * char_w) / 2;
    if (title_x < 0) title_x = 0;
    int title_y = (MENU_TOP_BAR_H - char_h) / 2;
    if (title_y < 0) title_y = 0;
    gfx->draw_text(gfx->user, title_x, title_y, lvl->title);

    // separator line
    draw_hline(gfx, 0, MENU_TOP_BAR_H, w);

    // draw menu items
    int content_y = MENU_TOP_BAR_H + 1;
    int max_visible = (h - content_y) / MENU_ITEM_H;
    if (max_visible > MENU_VISIBLE_ITEMS) max_visible = MENU_VISIBLE_ITEMS;
    if (max_visible > lvl->count) max_visible = lvl->count;

    for (int i = 0; i < max_visible; ++i) {
        int item_idx = s_scroll_offset + i;
        if (item_idx >= lvl->count) break;

        const menu_item_t *item = &lvl->items[item_idx];
        int item_y = content_y + i * MENU_ITEM_H;
        bool selected = (item_idx == s_selected_index);

        if (selected) {
            draw_inverted_rect(gfx, 0, item_y, w, MENU_ITEM_H);
        }

        int icon_y = item_y + (MENU_ITEM_H - MENU_ICON_SIZE) / 2;
        if (selected) {
            // draw icon inverted
            for (int r = 0; r < 8; ++r) {
                uint8_t bits = item->icon ? item->icon[r] : 0;
                for (int c = 0; c < 8; ++c) {
                    bool on = (bits >> (7 - c)) & 0x01;
                    for (int sy = 0; sy < gfx->scale_y; ++sy) {
                        gfx->plot_pixel(gfx->user, 2 + c, icon_y + r * gfx->scale_y + sy, !on);
                    }
                }
            }
        } else {
            draw_icon(gfx, 2, icon_y, item->icon);
        }

        int text_y = item_y + (MENU_ITEM_H - char_h) / 2;
        if (text_y < item_y) text_y = item_y;
        gfx->draw_text(gfx->user, MENU_TEXT_X, text_y, item->label);
    }

    // scroll indicators
    if (s_scroll_offset > 0) {
        int arrow_x = w / 2;
        int arrow_y = content_y - 2;
        gfx->plot_pixel(gfx->user, arrow_x, arrow_y, true);
        gfx->plot_pixel(gfx->user, arrow_x - 1, arrow_y - 1, true);
        gfx->plot_pixel(gfx->user, arrow_x + 1, arrow_y - 1, true);
    }
    if (s_scroll_offset + max_visible < lvl->count) {
        int arrow_x = w / 2;
        int arrow_y = h - 2;
        gfx->plot_pixel(gfx->user, arrow_x, arrow_y, true);
        gfx->plot_pixel(gfx->user, arrow_x - 1, arrow_y + 1, true);
        gfx->plot_pixel(gfx->user, arrow_x + 1, arrow_y + 1, true);
    }
}

const menu_level_t *status_menu_get_current(void) {
    if (!s_menu_active || s_current_level >= (int)NUM_LEVELS) return NULL;
    return &levels[s_current_level];
}

int status_menu_get_selected(void) {
    return s_selected_index;
}

int status_menu_get_scroll_offset(void) {
    return s_scroll_offset;
}

#endif // CONFIG_WITH_STATUS_DISPLAY
