#ifndef STATUS_DISPLAY_MENU_H
#define STATUS_DISPLAY_MENU_H

#include <stdbool.h>
#include <stdint.h>

#define MENU_MAX_ITEMS 12
#define MENU_MAX_SUBMENUS 4
#define MENU_MAX_LABEL_LEN 22
#define MENU_ICON_SIZE 8

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_BACK,
    MENU_ACTION_WIFI_SCAN,
    MENU_ACTION_WIFI_STATION_SCAN,
    MENU_ACTION_WIFI_DEAUTH,
    MENU_ACTION_WIFI_BEACON,
    MENU_ACTION_WIFI_BEACON_RICKROLL,
    MENU_ACTION_WIFI_EAPOL,
    MENU_ACTION_WIFI_KARMA,
    MENU_ACTION_BLE_GATT_SCAN,
    MENU_ACTION_BLE_FLIPPER_SCAN,
    MENU_ACTION_BLE_AIRTAG_SCAN,
    MENU_ACTION_BLE_ADVERTISER_SCAN,
    MENU_ACTION_BLE_RAW_SCAN,
    MENU_ACTION_BLE_SPAM_APPLE,
    MENU_ACTION_BLE_SPAM_SAMSUNG,
    MENU_ACTION_BLE_SPAM_GOOGLE,
    MENU_ACTION_NFC_SCAN,
    MENU_ACTION_NFC_EMULATE,
    MENU_ACTION_SWEEP,
    MENU_ACTION_SETTINGS,
    MENU_ACTION_INFO,
    MENU_ACTION_STOP_ALL,
} menu_action_t;

typedef struct {
    const char *label;
    const uint8_t *icon;
    menu_action_t action;
} menu_item_t;

typedef struct {
    const char *title;
    const menu_item_t *items;
    int count;
} menu_level_t;

typedef void (*menu_plot_pixel_fn)(void *user, int x, int y, bool on);
typedef void (*menu_draw_text_fn)(void *user, int x, int y, const char *text);

typedef struct {
    menu_plot_pixel_fn plot_pixel;
    menu_draw_text_fn draw_text;
    void *user;
    int width;
    int height;
    int scale_y;
    int font_char_width;
} menu_gfx_t;

void status_menu_init(void);
bool status_menu_is_active(void);
void status_menu_activate(void);
void status_menu_deactivate(void);
void status_menu_handle_press(void);
void status_menu_handle_long_press(void);
void status_menu_render(const menu_gfx_t *gfx);

const menu_level_t *status_menu_get_current(void);
int status_menu_get_selected(void);

#endif // STATUS_DISPLAY_MENU_H
