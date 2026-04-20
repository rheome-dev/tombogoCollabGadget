/**
 * Dead Front Display Test - LVGL Version
 *
 * Uses LVGL for proper double-buffered rendering to eliminate tearing.
 * Uses existing HAL infrastructure to avoid SPI bus conflicts.
 */

#ifdef PLATFORM_ESP32

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include "hal/hal.h"
#include "hal/display_hal.h"
#include "hal/touch_hal.h"
#include "core/lvgl_driver.h"
#include "config.h"

// Circle properties
static const int16_t CIRCLE_RADIUS = 70;
static const float LERP_FACTOR = 0.5f;

// State
static float circleX = SCREEN_WIDTH / 2.0f;
static float circleY = SCREEN_HEIGHT / 2.0f;
static float targetX = SCREEN_WIDTH / 2.0f;
static float targetY = SCREEN_HEIGHT / 2.0f;
static bool wasPressed = false;

// LVGL objects
static lv_obj_t* circle = nullptr;

// Forward declarations
static void createUI(void);
static void updateCirclePosition(void);

void deadFrontDisplayTest(void) {
    Serial.println("\n=== Dead Front Display Test (LVGL) ===");

    // Initialize full HAL (includes TCA9554, display, touch)
    Serial.println("Initializing HAL...");
    HAL_init();

    // Wait for display to be ready
    if (!DisplayHAL_isReady()) {
        Serial.println("ERROR: Display not ready!");
        while(1) delay(1000);
    }
    Serial.println("Display ready");

    // Clear display completely (multiple times to ensure no artifacts)
    DisplayHAL_fillScreen(0x0000);  // BLACK
    delay(50);
    DisplayHAL_fillScreen(0x0000);
    delay(50);

    // Initialize LVGL driver
    Serial.println("Initializing LVGL...");
    if (!LVGL_driver_init()) {
        Serial.println("ERROR: LVGL init failed!");
        while(1) delay(1000);
    }
    Serial.println("LVGL ready");

    // Extra display clear to remove any artifacts
    DisplayHAL_fillScreen(0x0000);
    delay(100);
    DisplayHAL_fillScreen(0x0000);

    // Check touch
    if (TouchHAL_isReady()) {
        Serial.println("Touch ready");
    } else {
        Serial.println("WARNING: Touch not ready!");
    }

    // Create the UI
    createUI();

    Serial.println("Test ready! Touch to move circle");
    Serial.printf("Screen: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);

    // Main loop
    uint32_t lastTick = millis();
    while (true) {
        // Update LVGL tick
        uint32_t now = millis();
        uint32_t elapsed = now - lastTick;
        lastTick = now;
        LVGL_driver_tick(elapsed);

        // Process touch and update circle position
        updateCirclePosition();

        // Run LVGL task handler
        uint32_t sleepTime = LVGL_driver_task();

        // Small delay (LVGL returns suggested sleep time)
        delay(sleepTime > 16 ? 16 : sleepTime);
    }
}

static void createUI(void) {
    // Get active screen
    lv_obj_t* scr = lv_scr_act();

    // Set screen background to black
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Create the draggable circle using an object with rounded corners
    circle = lv_obj_create(scr);
    lv_obj_set_size(circle, CIRCLE_RADIUS * 2, CIRCLE_RADIUS * 2);
    lv_obj_set_style_radius(circle, CIRCLE_RADIUS, 0);  // Fully rounded = circle
    lv_obj_set_style_bg_color(circle, lv_color_make(150, 220, 255), 0);  // Bright cyan-blue
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_scrollbar_mode(circle, LV_SCROLLBAR_MODE_OFF);

    // Position circle at center
    lv_obj_set_pos(circle, (int16_t)(circleX - CIRCLE_RADIUS), (int16_t)(circleY - CIRCLE_RADIUS));
}

static void updateCirclePosition(void) {
    if (!TouchHAL_isReady()) return;

    uint8_t touchCount = TouchHAL_getCount();

    if (touchCount > 0) {
        TouchPoint point;
        if (TouchHAL_getPoint(0, &point)) {
            int16_t rawX = point.x;
            int16_t rawY = point.y;

            // Coordinate transformation for this display/touch combo
            // The touch panel orientation is inverted from display orientation
            // Touch in bottom-right shows circle in top-left → flip both axes
            targetX = (float)(SCREEN_WIDTH - 1 - rawX);
            targetY = (float)(SCREEN_HEIGHT - 1 - rawY);

            // Clamp target to valid range (circle radius from edges)
            if (targetX < CIRCLE_RADIUS) targetX = CIRCLE_RADIUS;
            if (targetY < CIRCLE_RADIUS) targetY = CIRCLE_RADIUS;
            if (targetX > SCREEN_WIDTH - CIRCLE_RADIUS) targetX = SCREEN_WIDTH - CIRCLE_RADIUS;
            if (targetY > SCREEN_HEIGHT - CIRCLE_RADIUS) targetY = SCREEN_HEIGHT - CIRCLE_RADIUS;

            if (!wasPressed) {
                Serial.printf("Touch: raw(%d, %d) -> target(%.0f, %.0f)\n",
                    rawX, rawY, targetX, targetY);
            }
            wasPressed = true;
        }
    } else {
        wasPressed = false;
    }

    // Lerp toward target
    float dx = targetX - circleX;
    float dy = targetY - circleY;

    if (fabsf(dx) > 0.5f || fabsf(dy) > 0.5f) {
        circleX += dx * LERP_FACTOR;
        circleY += dy * LERP_FACTOR;

        // Clamp circle center to stay within screen bounds (accounting for radius)
        if (circleX < CIRCLE_RADIUS) circleX = CIRCLE_RADIUS;
        if (circleY < CIRCLE_RADIUS) circleY = CIRCLE_RADIUS;
        if (circleX > SCREEN_WIDTH - CIRCLE_RADIUS) circleX = SCREEN_WIDTH - CIRCLE_RADIUS;
        if (circleY > SCREEN_HEIGHT - CIRCLE_RADIUS) circleY = SCREEN_HEIGHT - CIRCLE_RADIUS;

        // Update circle position in LVGL
        int16_t posX = (int16_t)(circleX - CIRCLE_RADIUS);
        int16_t posY = (int16_t)(circleY - CIRCLE_RADIUS);

        lv_obj_set_pos(circle, posX, posY);
        // Force full screen redraw to prevent trails/flickering
        lv_obj_invalidate(lv_scr_act());
    }
}

#endif // PLATFORM_ESP32
