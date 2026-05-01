/**
 * ESP32 Platform - Touch HAL Implementation
 *
 * Uses SensorLib for CST9217/FT3168 capacitive touch controller
 * Reference: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
 */

#ifdef PLATFORM_ESP32

#include "../../hal/touch_hal.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <TouchDrvCSTXXX.hpp>

// Touch controller object
static TouchDrvCSTXXX* touch = nullptr;

// Touch state
static bool touchAvailable = false;
static TouchPoint touchPoints[5];  // Max 5 touch points
static uint8_t touchCount = 0;

// Screen dimensions for coordinate mapping
static int16_t screenWidth = LCD_WIDTH;
static int16_t screenHeight = LCD_HEIGHT;

void TouchHAL_init(void) {
    Serial.println("ESP32: Initializing Touch HAL...");
    Serial.println("ESP32: Note - Touch reset should already be done via TCA9554");

    // Note: I2C is already initialized in HAL_init()
    // Note: Touch reset is already done via TCA9554_resetTouch()

    // Create CSTXXX touch controller instance
    touch = new TouchDrvCSTXXX();

    // Initialize the touch controller
    // Try CST9217 first (Waveshare default)
    if (!touch->begin(Wire, CST9217_ADDR, screenWidth, screenHeight)) {
        Serial.printf("ESP32: CST9217 not found at 0x%02X, trying FT3168...\n", CST9217_ADDR);
        // Try alternative address FT3168
        if (!touch->begin(Wire, 0x38, screenWidth, screenHeight)) {
            Serial.println("ESP32: Touch controller init failed!");
            delete touch;
            touch = nullptr;
            touchAvailable = false;
            return;
        }
        Serial.println("ESP32: FT3168 touch controller found at 0x38");
    } else {
        Serial.printf("ESP32: CST9217 touch controller found at 0x%02X\n", CST9217_ADDR);
    }

    touchAvailable = true;

    Serial.println("ESP32: Touch initialized successfully");
    Serial.printf("  Screen size: %dx%d\n", screenWidth, screenHeight);
}

uint8_t TouchHAL_getCount(void) {
    if (!touchAvailable || !touch) {
        return 0;
    }

    // Read touch points from controller
    int16_t x[5], y[5];
    uint8_t count = touch->getPoint(x, y, 5);

    touchCount = count;

    // Map raw controller coords into LVGL's logical frame.
    // LVGL is configured with LV_DISP_ROT_270 (sw_rotate), and 8.x does NOT
    // auto-rotate input devices — the read callback must do it. For a square
    // 466x466 panel rotated 270° CW the inverse mapping is:
    //   xL = (W - 1) - yT
    //   yL = xT
    for (uint8_t i = 0; i < 5; i++) {
        if (i < count) {
            touchPoints[i].x = (int16_t)(screenWidth - 1) - y[i];
            touchPoints[i].y = x[i];
            touchPoints[i].pressed = true;
        } else {
            touchPoints[i].pressed = false;
        }
    }

    return count;
}

bool TouchHAL_getPoint(uint8_t index, TouchPoint* point) {
    if (!touchAvailable || !point || index >= 5) {
        return false;
    }

    // Don't call getCount() again - the data was already fetched by getCount()
    // Calling getCount() again causes double-read which can clear CST9217 touch buffer
    if (index < touchCount) {
        *point = touchPoints[index];
        return true;
    }

    return false;
}

bool TouchHAL_isPressed(void) {
    return TouchHAL_getCount() > 0;
}

bool TouchHAL_isReady(void) {
    return touchAvailable && touch != nullptr;
}

uint8_t TouchHAL_getCount_raw(void) {
    if (!touchAvailable || !touch) return 0;
    int16_t x[5], y[5];
    return touch->getPoint(x, y, 5);
}

bool TouchHAL_getPoint_raw(uint8_t index, int16_t* x, int16_t* y) {
    if (!touchAvailable || !touch || index >= 5) return false;
    int16_t tmpX[5], tmpY[5];
    uint8_t count = touch->getPoint(tmpX, tmpY, 5);
    if (index < count) {
        if (x) *x = tmpX[index];
        if (y) *y = tmpY[index];
        return true;
    }
    return false;
}

#endif // PLATFORM_ESP32
