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
    delay(7000);  // Allow time to connect serial monitor before boot output

    // Kill WiFi/BT radio ASAP — current spikes from the radio modulate
    // the power rail and inject noise into ES8311/ES7210 analog sections.
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_bt_mem_release(ESP_BT_MODE_BTDM);
    Serial.println("Radio: WiFi + BT disabled (audio noise reduction)");

    Serial.println("\n=== Tombogo Collab Gadget v0.3 ===");
    Serial.println("Platform: ESP32-S3 (Waveshare AMOLED)");

    // Force PSRAM initialization and detection FIRST
    if (ESP.getPsramSize() == 0) {
        Serial.println("PSRAM: Detecting...");
        esp_err_t err = esp_spiram_init();
        if (err == ESP_OK) {
            uint32_t psramSize = esp_spiram_get_size();
            Serial.printf("PSRAM: Initialized - %u bytes\n", psramSize);
        } else {
            Serial.printf("PSRAM: Init failed (0x%x) - buffer allocation will fail\n", err);
        }
    } else {
        Serial.printf("PSRAM: %u bytes available\n", ESP.getPsramSize());
    }
    Serial.printf("PSRAM status: %u bytes free, %u total\n",
                   ESP.getFreePsram(), ESP.getPsramSize());
    Serial.println("Setup starting...");

    // Initialize hardware abstraction layer
    HAL_init();
    Serial.printf("  After HAL init   - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Initialize storage (SD card)
    StorageHAL_init();
    Serial.printf("  After storage    - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Initialize audio engine (includes retroactive buffer)
    AudioEngine_init();
    Serial.printf("  After audio      - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Initialize stage manager
    StageManager_init();
    Serial.printf("  After stage      - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Initialize BPM clock
    BPMClock_init(120);
    BPMClock_start();
    Serial.printf("  After BPM clock  - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Initialize UI
    UIManager_init();
    Serial.printf("  After UI         - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Start input task (Core 0, 10ms polling)
    InputTask_start();
    Serial.printf("  After input      - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Start audio hardware (enables PA, ES8311, ES7210)
    AudioHAL_start();

    // Start audio task on Core 1 BEFORE diagnostic — the audio task keeps the
    // I2S DMA continuously fed with i2s_write() calls, which is the only thing
    // that keeps BCLK/WS toggling in the legacy ESP-IDF 4.4.7 driver.
    // Without the task running, the pre-filled DMA zero buffers drain and the
    // clocks stop, giving false "NOT TOGGLING" results in the diagnostic.
    AudioEngine_startTask();

    // Let the audio task stabilize (fill DMA buffers and establish steady clocks)
    delay(200);
    AudioHAL_runDiagnostic();
    Serial.printf("  After audio task - Heap: %u, PSRAM: %u\n",
                   ESP.getFreeHeap(), ESP.getFreePsram());

    // Test tone disabled (uncomment to test speaker/DAC path)
    // AudioEngine_playTestTone();

    Serial.println("\n=== Ready! ===");
}

void loop() {
    StageManager_update();
    UIManager_update();

    delay(MAIN_LOOP_DELAY_MS);
}
