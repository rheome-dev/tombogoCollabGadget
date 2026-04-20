/**
 * Tombogo Collab Gadget - Main Application Entry
 *
 * PlatformIO entry point for ESP32-S3 prototype
 * Modular firmware architecture for ESP32-S3 prototype → Custom PCB
 */

#include <Arduino.h>
#include "config.h"
#include "hal/hal.h"
#include "hal/display_hal.h"
#include "hal/audio_hal.h"
#include "hal/imu_hal.h"
#include "hal/touch_hal.h"
#include "hal/storage_hal.h"
#include "hal/button_hal.h"
#include "core/audio_engine.h"
#include "core/ui_manager.h"
#include "core/retroactive_buffer.h"

// FreeRTOS task handles (if using)
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t imuTaskHandle = NULL;
TaskHandle_t uiTaskHandle = NULL;

// Global state
SystemState g_systemState;

// ─── Button Callback ────────────────────────────────────────────────────────
// Record button (P0 on PCF8574): tap = capture + start looping playback
// Tap again while playing = stop playback

static void onButtonEvent(ButtonId id, ButtonState state) {
    if (state != BUTTON_PRESSED) return;  // Ignore release / long-press for now

    if (id == BUTTON_RECORD) {
        if (RetroactiveBuffer_isPlaying()) {
            // Stop playback on second tap
            RetroactiveBuffer_stopPlayback();
            AudioEngine_setPlaying(false);
        } else {
            // First tap: capture what's in the buffer, then loop it
            AudioEngine_triggerCapture();
            RetroactiveBuffer_startPlayback();
            AudioEngine_setPlaying(true);
        }
    }
    // Future: other buttons for stop/clear/resonate
}

void setup() {
    Serial.begin(115200);
    delay(100);  // Give serial time to stabilize

    Serial.println("\n=== Tombogo Collab Gadget v0.1 ===");
    Serial.println("Platform: ESP32-S3 (Waveshare AMOLED)");
    Serial.printf("PSRAM: %s\n", psramFound() ? "Available" : "Not found");
    Serial.println("Setup starting...");

    // Initialize hardware abstraction layer
    HAL_init();

    // Initialize storage (SD card)
    StorageHAL_init();

    // Initialize buttons (PCF8574)
    ButtonHAL_init();
    if (ButtonHAL_isConnected()) {
        Serial.println("Buttons: PCF8574 connected");
        ButtonHAL_onPress(onButtonEvent);
    } else {
        Serial.println("Buttons: PCF8574 not found (optional)");
    }

    // Initialize audio engine
    AudioEngine_init();

    // Initialize retroactive buffer
    RetroactiveBuffer_init();

    // Initialize UI
    UIManager_init();

    Serial.println("\n=== Ready! ===");
}

void loop() {
    // Update button states
    ButtonHAL_update();

    // UI updates on main loop
    UIManager_update();

    delay(MAIN_LOOP_DELAY_MS);
}
