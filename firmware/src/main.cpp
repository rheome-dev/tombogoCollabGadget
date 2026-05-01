/**
 * Tombogo Collab Gadget - Main Application Entry
 *
 * PlatformIO entry point for ESP32-S3 prototype
 * Modular firmware architecture for ESP32-S3 prototype → Custom PCB
 */

#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include "config.h"
#include "hal/hal.h"
#include "hal/display_hal.h"
#include "hal/audio_hal.h"
#include "hal/imu_hal.h"
#include "hal/touch_hal.h"
#include "hal/storage_hal.h"
#include "core/audio_engine.h"
#include "core/ui_manager.h"
#include "core/retroactive_buffer.h"
#include "core/stage_manager.h"
#include "input/input_task.h"
#include "core/bpm_clock.h"

void setup() {
    Serial.begin(115200);

    // Kill WiFi/BT radio ASAP — current spikes from the radio modulate
    // the power rail and inject noise into ES8311/ES7210 analog sections.
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_bt_mem_release(ESP_BT_MODE_BTDM);

    // Force PSRAM initialization if not auto-detected (silent unless it fails)
    if (ESP.getPsramSize() == 0) {
        esp_err_t err = esp_spiram_init();
        if (err != ESP_OK) {
            Serial.printf("PSRAM: init failed (0x%x)\n", err);
        }
    }

    HAL_init();
    StorageHAL_init();
    AudioEngine_init();
    StageManager_init();
    BPMClock_init(120);
    BPMClock_start();
    UIManager_init();
    InputTask_start();

    // Audio hardware + task — task must start so DMA stays fed (legacy IDF
    // 4.4.7 stops BCLK/WS once pre-filled zero buffers drain).
    AudioHAL_start();
    AudioEngine_startTask();

    Serial.println("Ready");
}

void loop() {
    StageManager_update();
    UIManager_update();

    delay(MAIN_LOOP_DELAY_MS);
}
