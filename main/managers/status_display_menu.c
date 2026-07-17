#include "sdkconfig.h"

#ifdef CONFIG_WITH_STATUS_DISPLAY

#include "managers/status_display_menu.h"
#include <string.h>

#define ICON_SCALE 5
#define ICON_RENDER_SIZE (8 * ICON_SCALE)
#define TITLE_Y 2
#define ICON_Y 12
#define LABEL_Y (ICON_Y + ICON_RENDER_SIZE + 4)
#define DOTS_Y 60

static bool s_menu_active;
static int s_selected_index;
static int s_current_level;
static int s_level_stack[MENU_MAX_SUBMENUS];

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
    {"Station",      icon_station,  MENU_ACTION_WIFI_STATION_SCAN},
    {"Deauth",       icon_deauth,   MENU_ACTION_WIFI_DEAUTH},
    {"Beacon Spam",  icon_beacon,   MENU_ACTION_WIFI_BEACON},
    {"Rickroll",     icon_beacon,   MENU_ACTION_WIFI_BEACON_RICKROLL},
    {"EAPOL",        icon_deauth,   MENU_ACTION_WIFI_EAPOL},
    {"Karma",        icon_beacon,   MENU_ACTION_WIFI_KARMA},
};

static const menu_item_t ble_items[] = {
    {"Back",          icon_back,     MENU_ACTION_BACK},
    {"GATT Scan",     icon_scan,     MENU_ACTION_BLE_GATT_SCAN},
    {"Flipper",       icon_scan,     MENU_ACTION_BLE_FLIPPER_SCAN},
    {"AirTag",        icon_scan,     MENU_ACTION_BLE_AIRTAG_SCAN},
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
    {"Sweep",     icon_sweep,     MENU_ACTION_SWEEP},
    {"Stop",      icon_stop,      MENU_ACTION_STOP_ALL},
    {"Settings",  icon_settings,  MENU_ACTION_SETTINGS},
    {"Info",      icon_info,      MENU_ACTION_INFO},
};

static const menu_level_t levels[] = {
    {"GhostESP", main_items, sizeof(main_items) / sizeof(main_items[0])},
    {"WiFi",     wifi_items, sizeof(wifi_items) / sizeof(wifi_items[0])},
    {"BLE",      ble_items,  sizeof(ble_items)  / sizeof(ble_items[0])},
    {"NFC",      nfc_items,  sizeof(nfc_items)  / sizeof(nfc_items[0])},
};

#define NUM_LEVELS (sizeof(levels) / sizeof(levels[0]))

static void draw_scaled_icon(const menu_gfx_t *gfx, int cx, int cy, const uint8_t *icon) {
    if (!icon) return;
    int ox = cx - ICON_RENDER_SIZE / 2;
    int oy = cy - ICON_RENDER_SIZE / 2;
    for (int row = 0; row < 8; ++row) {
        uint8_t bits = icon[row];
        for (int col = 0; col < 8; ++col) {
            if (!((bits >> (7 - col)) & 0x01)) continue;
            int px = ox + col * ICON_SCALE;
            int py = oy + row * ICON_SCALE;
            for (int dy = 0; dy < ICON_SCALE; ++dy) {
                for (int dx = 0; dx < ICON_SCALE; ++dx) {
                    gfx->plot_pixel(gfx->user, px + dx, py + dy, true);
                }
            }
        }
    }
}

void status_menu_init(void) {
    s_menu_active = false;
    s_selected_index = 0;
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
    }
}

void status_menu_handle_long_press(void) {
    if (!s_menu_active) return;
    const menu_level_t *lvl = &levels[s_current_level];
    if (s_selected_index < 0 || s_selected_index >= lvl->count) return;
    const menu_item_t *item = &lvl->items[s_selected_index];

    if (item->action == MENU_ACTION_BACK) {
        if (s_current_level > 0) {
            s_current_level--;
            s_selected_index = s_level_stack[s_current_level];
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
    int char_w = gfx->font_char_width + 1;

    int title_len = (int)strlen(lvl->title);
    int title_x = (w - title_len * char_w) / 2;
    if (title_x < 0) title_x = 0;
    gfx->draw_text(gfx->user, title_x, TITLE_Y, lvl->title);

    const menu_item_t *item = &lvl->items[s_selected_index];
    draw_scaled_icon(gfx, w / 2, ICON_Y + ICON_RENDER_SIZE / 2, item->icon);

    int label_len = (int)strlen(item->label);
    int label_x = (w - label_len * char_w) / 2;
    if (label_x < 0) label_x = 0;
    gfx->draw_text(gfx->user, label_x, LABEL_Y, item->label);

    int total = lvl->count;
    int dot_spacing = 4;
    int dots_total_w = total * dot_spacing;
    int dots_start_x = (w - dots_total_w) / 2;
    for (int i = 0; i < total; ++i) {
        int dx = dots_start_x + i * dot_spacing;
        bool active = (i == s_selected_index);
        gfx->plot_pixel(gfx->user, dx, DOTS_Y, active);
        if (active) {
            gfx->plot_pixel(gfx->user, dx - 1, DOTS_Y, true);
            gfx->plot_pixel(gfx->user, dx + 1, DOTS_Y, true);
        }
    }
}

const menu_level_t *status_menu_get_current(void) {
    if (!s_menu_active || s_current_level >= (int)NUM_LEVELS) return NULL;
    return &levels[s_current_level];
}

int status_menu_get_selected(void) {
    return s_selected_index;
}

#endif // CONFIG_WITH_STATUS_DISPLAY
