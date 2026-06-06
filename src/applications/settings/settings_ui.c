#include <managers/app_manager.h>
#include "assets/setting_ico.h"
#include <common_types.h>
#include "lvgl.h"

// Hardware and Panel Constraints
#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT    160
#define PANEL_WIDTH      128
#define PANEL_HEIGHT     40

// Custom color palette for high-contrast wearable display
#define COLOR_PANEL_BG   lv_color_make(0x16, 0x16, 0x1A) // Deep Obsidian Black
#define COLOR_ACCENT     lv_color_make(0x00, 0xE5, 0xFF) // High-visibility Cyan
#define COLOR_DIVIDER    lv_color_make(0x33, 0x33, 0x37) // Dark Slate Gray
#define COLOR_TEXT_BODY  lv_color_make(0xEE, 0xEE, 0xEE) // Off-White (reduces glare)

/**
 * @brief Displays a 128x40 notification panel on a fixed 128x160 viewport.
 * @param root Pointer to the root container or screen (e.g., lv_scr_act()).
 * @param app_name String for the header line.
 * @param message String for the body text line.
 */
void display_notification(lv_obj_t *root, const char *app_name, const char *message)
{
    if (root == NULL) return;

    // Fixed absolute layout geometry 
    lv_coord_t final_x = 0;                          // Spans the full width of the screen
    lv_coord_t final_y = SCREEN_HEIGHT - PANEL_HEIGHT; // 160 - 40 = 120px (Resting baseline)
    
    // Animation vector limits
    lv_coord_t start_y = SCREEN_HEIGHT;              // 160px (Completely off-screen at bottom)
    lv_coord_t peak_y  = final_y - 10;               // 110px (Overshoots 10px upward into view)

    // 1. Create Base Notification Panel
    lv_obj_t *panel = lv_obj_create(root);
    lv_obj_remove_style_all(panel); 
    lv_obj_set_size(panel, PANEL_WIDTH, PANEL_HEIGHT);
    lv_obj_set_pos(panel, final_x, start_y); // Set initial position below viewport

    // Style the panel background
    static lv_style_t style_panel;
    lv_style_init(&style_panel);
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_bg_color(&style_panel, COLOR_PANEL_BG);
    lv_style_set_radius(&style_panel, 6); // Slightly rounder top corners
    lv_obj_add_style(panel, &style_panel, 0);

    // 2. Add App Icon Placeholder (Centered vertically inside the 40px height)
    lv_obj_t *icon = lv_label_create(panel);
    lv_label_set_text(icon, LV_SYMBOL_BELL); 
    lv_obj_set_style_text_color(icon, COLOR_ACCENT, 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 6, 0); // 6px padding from left edge

    // 3. Draw a Vertical Separator Line (Scaled to 30px height for the 40px panel)
    lv_obj_t *divider = lv_obj_create(panel);
    lv_obj_remove_style_all(divider);
    lv_obj_set_size(divider, 1, 30);
    lv_obj_set_pos(divider, 25, 5); // Shifted right to clear icon padding safely
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(divider, COLOR_DIVIDER, 0);

    // 4. Create Header Line (App Name / Sender)
    // Dynamic text width boundary is now 93px (128 total - 25 divider - 10 padding)
    lv_obj_t *lbl_header = lv_label_create(panel);
    lv_label_set_text(lbl_header, app_name);
    lv_obj_set_size(lbl_header, 93, 10); 
    lv_obj_set_pos(lbl_header, 30, 4); // Placed comfortably at the top right quadrant
    lv_label_set_long_mode(lbl_header, LV_LABEL_LONG_DOT); 
    lv_obj_set_style_text_color(lbl_header, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_header, &lv_font_montserrat_10, 0);

    // 5. Create Message Body Line
    lv_obj_t *lbl_body = lv_label_create(panel);
    lv_label_set_text(lbl_body, message);
    lv_obj_set_size(lbl_body, 93, 20);
    lv_obj_set_pos(lbl_body, 30, 20); // Anchored on the lower half of the 40px height
    lv_label_set_long_mode(lbl_body, LV_LABEL_LONG_DOT); 
    lv_obj_set_style_text_color(lbl_body, COLOR_TEXT_BODY, 0);
    lv_obj_set_style_text_font(lbl_body, &lv_font_montserrat_10, 0);

    // 6. Configure the Jump and Stabilize Animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, panel);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    
    // Animate from 160 down to 110 (LVGL will snap back down to rest at 120)
    lv_anim_set_values(&a, start_y, peak_y);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_time(&a, 400); // Sharp, 400ms spring physics

    // Execute animation
    lv_anim_start(&a);
}


static void draw_app_ui(lv_obj_t* parent_obj) {
    display_notification(parent_obj, "Sourav", "Test message");
}

static void delete_app_ui(lv_obj_t* parent_obj) {

}

static application_t settings_app = {
    .name = "Settings",
    .ico = &setting_ico,
    .draw_app = draw_app_ui,
    .close_app = delete_app_ui,
};

static int register_app(void) {
    add_app(&settings_app);
    return 0;
}

SYS_INIT(register_app, APPLICATION, APP_PRIORITY);

