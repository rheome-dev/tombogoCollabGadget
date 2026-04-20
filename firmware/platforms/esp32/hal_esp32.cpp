/**
 * ESP32 Platform - HAL Initialization
 *
 * Initializes all platform-specific HAL components
 * Critical: TCA9554 must initialize FIRST - it controls display/touch power
 */

#ifdef PLATFORM_ESP32

#include "../../hal/hal.h"
#include "../../hal/display_hal.h"
#include "../../hal/audio_hal.h"
#include "../../hal/imu_hal.h"
#include "../../hal/touch_hal.h"
#include "../../hal/storage_hal.h"
#include "../../hal/button_hal.h"
#include "tca9554.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>

void HAL_init(void) {
    Serial.println("Initializing HAL...");

    // Initialize I2C bus FIRST (shared by TCA9554, codec, IMU, touch)
    Wire.begin((int)I2C_SDA, (int)I2C_SCL, 400000);
    Serial.printf("ESP32: I2C initialized (SDA=%d, SCL=%d)\n", I2C_SDA, I2C_SCL);

    // Initialize TCA9554 IO expander FIRST - controls LCD power and resets
    if (!TCA9554_init()) {
        Serial.println("WARNING: TCA9554 init failed - display/touch may not work!");
    }

    // Power on LCD via TCA9554 (must happen before display init)
    TCA9554_powerOnLCD();

    // Reset touch controller via TCA9554
    TCA9554_resetTouch();

    // Now initialize display (hardware is powered on)
    DisplayHAL_init();

    // Initialize audio (includes ES8311 codec init)
    AudioHAL_init();

    // Initialize IMU
    IMUHAL_init();

    // Initialize touch (hardware has been reset)
    TouchHAL_init();

    Serial.println("HAL initialization complete");
}

#endif // PLATFORM_ESP32
