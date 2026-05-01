/**
 * UI Manager Implementation
 *
 * Manages the user interface using LVGL
 */

#include "ui_manager.h"
#include "stage_manager.h"
#include "../config.h"
#include "lvgl_driver.h"
#include "../../hal/display_hal.h"
#include "../../hal/touch_hal.h"
#include "../ui/resonate_view.h"
#include <Arduino.h>
#include <lvgl.h>

// LVGL tick timing
static uint32_t lastTickTime = 0;
static const uint32_t TICK_PERIOD_MS = 5;

// UI state
static bool uiInitialized = false;

// UI elements
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* statusLabel = nullptr;
static lv_obj_t* stageLabel = nullptr;
static Stage lastDisplayedStage = STAGE_IDLE;

/**
 * Create the initial UI layout
 */
static void create_ui(void) {
    // Get the active screen
    mainScreen = lv_scr_act();

    // Set background color (dark for AMOLED)
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mainScreen, LV_OPA_COVER, LV_PART_MAIN);

    // Clip to circle (round 466px display, radius = 233)
    lv_obj_set_style_radius(mainScreen, 233, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(mainScreen, true, LV_PART_MAIN);

    // Title
    statusLabel = lv_label_create(mainScreen);
    lv_label_set_text(statusLabel, "TOMBOGO");
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x00ffcc), LV_PART_MAIN);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -60);

    // Stage indicator (large, centered)
    stageLabel = lv_label_create(mainScreen);
    lv_label_set_text(stageLabel, "IDLE");
    lv_obj_set_style_text_color(stageLabel, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(stageLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(stageLabel, LV_ALIGN_CENTER, 0, 20);

    Serial.println("UI: Main screen created (round clipping enabled)");
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

    // Build per-stage views (start hidden, shown on stage entry)
    ResonateView_create();

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

    // Update stage display if changed
    Stage current = StageManager_current();
    if (current != lastDisplayedStage && stageLabel) {
        lastDisplayedStage = current;
        const char* name = StageManager_stageName();
        lv_label_set_text(stageLabel, name);

        // Color per stage
        uint32_t color = 0xffffff;
        switch (current) {
            case STAGE_IDLE:           color = 0x808080; break;
            case STAGE_CAPTURE_REVIEW: color = 0x00ff88; break;
            case STAGE_CHOP:           color = 0xff4444; break;
            case STAGE_RESONATE:       color = 0x4488ff; break;
            default: break;
        }
        lv_obj_set_style_text_color(stageLabel, lv_color_hex(color), LV_PART_MAIN);
    }

    // Mirror audio-task-applied chord into the LVGL view (LVGL must be
    // touched only from this UI thread, not the audio task).
    StageManager_update();

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
