/**
 * UI Manager Implementation
 *
 * Manages the user interface using LVGL
 */

#include "ui_manager.h"
#include "../config.h"
#include "lvgl_driver.h"
#include "../../hal/display_hal.h"
#include "../../hal/touch_hal.h"
#include <Arduino.h>
#include <lvgl.h>

// LVGL tick timing
static uint32_t lastTickTime = 0;
static const uint32_t TICK_PERIOD_MS = 5;

// UI state
static bool uiInitialized = false;

// UI elements (example - expand as needed)
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* statusLabel = nullptr;

/**
 * Create the initial UI layout
 */
static void create_ui(void) {
    // Get the active screen
    mainScreen = lv_scr_act();

    // Set background color (dark blue/purple for retro aesthetic)
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    // Create a simple status label
    statusLabel = lv_label_create(mainScreen);
    lv_label_set_text(statusLabel, "Tombogo Gadget");
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x00ffcc), LV_PART_MAIN);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -180);

    // Create subtitle
    lv_obj_t* subtitleLabel = lv_label_create(mainScreen);
    lv_label_set_text(subtitleLabel, "Touch to interact");
    lv_obj_set_style_text_color(subtitleLabel, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_align(subtitleLabel, LV_ALIGN_CENTER, 0, -140);

    Serial.println("UI: Main screen created");
}

void UIManager_init() {
    Serial.println("Initializing UI Manager...");

    // Initialize LVGL driver (connects to display and touch HALs)
    if (!LVGL_driver_init()) {
        Serial.println("UI Manager: LVGL driver init failed!");
        // Fall back to basic display
        DisplayHAL_fillScreen(0x1082);  // Dark blue
        return;
    }

    // Create the UI
    create_ui();

    // Initialize tick timer
    lastTickTime = millis();

    uiInitialized = true;
    Serial.println("UI Manager initialized with LVGL");
}

void UIManager_update() {
    if (!uiInitialized) {
        return;
    }

    // Update LVGL tick
    uint32_t now = millis();
    uint32_t elapsed = now - lastTickTime;
    if (elapsed >= TICK_PERIOD_MS) {
        LVGL_driver_tick(elapsed);
        lastTickTime = now;
    }

    // Run LVGL task handler (processes drawing, animations, input)
    LVGL_driver_task();
}

void UIManager_handleIMU(IMUData imuData) {
    // Could update UI elements based on IMU data
    // For example, show tilt visualization or update parameters

    // Placeholder: update status label with IMU info (remove in production)
    if (statusLabel && uiInitialized) {
        // Don't update too frequently to avoid flicker
        static uint32_t lastUpdate = 0;
        if (millis() - lastUpdate > 100) {
            // char buf[64];
            // snprintf(buf, sizeof(buf), "Pitch: %.1f  Roll: %.1f", imuData.pitch, imuData.roll);
            // lv_label_set_text(statusLabel, buf);
            lastUpdate = millis();
        }
    }
}

void UIManager_setStatus(const char* status) {
    if (statusLabel && uiInitialized) {
        lv_label_set_text(statusLabel, status);
    }
}
