/**
 * ESP32 Button/Encoder Implementation
 *
 * Uses PCF8574 I2C GPIO expander for physical buttons
 * PCF8574 is optional - works without it but buttons won't be available
 *
 * Button mapping on PCF8574 (active low):
 *   P0 - BUTTON_RECORD
 *   P1 - BUTTON_STOP
 *   P2 - BUTTON_LOOP_SELECT
 *   P3 - BUTTON_EFFECT_BYPASS
 *   P4-P7 - Reserved for encoders (optional)
 */

#ifdef PLATFORM_ESP32

#include "../../hal/button_hal.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>

// Debounce settings
static const uint32_t DEBOUNCE_MS = 50;
static const uint32_t LONG_PRESS_MS = 500;
static const uint32_t DOUBLE_PRESS_MS = 300;

// Button state tracking
static struct {
    bool current;           // Current debounced state
    bool previous;          // Previous debounced state
    bool raw;               // Raw reading
    uint32_t lastChange;    // Time of last state change
    uint32_t pressTime;     // Time when button was pressed
    uint32_t releaseTime;   // Time when button was released
    uint8_t pressCount;     // Press count for double-press detection
} g_buttons[BUTTON_COUNT];

// Encoder state tracking
static struct {
    int32_t position;
    uint8_t lastState;
} g_encoders[ENCODER_COUNT];

// Callbacks
static ButtonCallback g_buttonCallback = NULL;
static EncoderCallback g_encoderCallback = NULL;

// Connection state
static bool g_connected = false;

// PCF8574 I/O functions
static bool pcf8574_write(uint8_t value) {
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static uint8_t pcf8574_read(void) {
    Wire.requestFrom((uint8_t)PCF8574_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;  // All high (no buttons pressed) on error
}

bool ButtonHAL_isConnected(void) {
    return g_connected;
}

void ButtonHAL_init(void) {
    Serial.println("ESP32: Initializing Button HAL (PCF8574)...");

    // Note: I2C is already initialized in HAL_init()

    // Check if PCF8574 is present
    Wire.beginTransmission(PCF8574_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.printf("ESP32: PCF8574 not found at address 0x%02X\n", PCF8574_ADDR);
        Serial.println("ESP32: Buttons will not be available (optional hardware)");
        g_connected = false;
        return;
    }

    // Set all pins as inputs (write high - PCF8574 uses quasi-bidirectional I/O)
    pcf8574_write(0xFF);

    // Initialize button states
    for (int i = 0; i < BUTTON_COUNT; i++) {
        g_buttons[i].current = false;
        g_buttons[i].previous = false;
        g_buttons[i].raw = false;
        g_buttons[i].lastChange = 0;
        g_buttons[i].pressTime = 0;
        g_buttons[i].releaseTime = 0;
        g_buttons[i].pressCount = 0;
    }

    // Initialize encoder states
    for (int i = 0; i < ENCODER_COUNT; i++) {
        g_encoders[i].position = 0;
        g_encoders[i].lastState = 0;
    }

    g_connected = true;
    Serial.println("ESP32: Button HAL initialized (PCF8574 connected)");
}

bool ButtonHAL_isPressed(ButtonId id) {
    if (id >= BUTTON_COUNT || !g_connected) return false;
    return g_buttons[id].current;
}

ButtonState ButtonHAL_getState(ButtonId id) {
    if (id >= BUTTON_COUNT || !g_connected) return BUTTON_RELEASED;

    if (!g_buttons[id].current) {
        return BUTTON_RELEASED;
    }

    // Check for long press
    uint32_t holdTime = millis() - g_buttons[id].pressTime;
    if (holdTime > LONG_PRESS_MS) {
        return BUTTON_LONG_PRESS;
    }

    return BUTTON_PRESSED;
}

int32_t ButtonHAL_getEncoder(EncoderId id) {
    if (id >= ENCODER_COUNT) return 0;
    return g_encoders[id].position;
}

void ButtonHAL_resetEncoder(EncoderId id) {
    if (id >= ENCODER_COUNT) return;
    g_encoders[id].position = 0;
}

void ButtonHAL_onPress(ButtonCallback callback) {
    g_buttonCallback = callback;
}

void ButtonHAL_onEncoderChange(EncoderCallback callback) {
    g_encoderCallback = callback;
}

void ButtonHAL_update(void) {
    if (!g_connected) return;

    uint32_t now = millis();

    // Read current state from PCF8574
    uint8_t pins = pcf8574_read();

    // Process each button
    for (int i = 0; i < BUTTON_COUNT; i++) {
        // PCF8574 buttons are active low
        bool rawState = !(pins & (1 << i));

        // Debouncing
        if (rawState != g_buttons[i].raw) {
            g_buttons[i].raw = rawState;
            g_buttons[i].lastChange = now;
        }

        // Apply debounced state
        if ((now - g_buttons[i].lastChange) > DEBOUNCE_MS) {
            bool newState = g_buttons[i].raw;

            // Detect state change
            if (newState != g_buttons[i].current) {
                g_buttons[i].previous = g_buttons[i].current;
                g_buttons[i].current = newState;

                if (newState) {
                    // Button pressed
                    g_buttons[i].pressTime = now;

                    // Check for double press
                    if ((now - g_buttons[i].releaseTime) < DOUBLE_PRESS_MS) {
                        g_buttons[i].pressCount++;
                    } else {
                        g_buttons[i].pressCount = 1;
                    }

                    // Call callback
                    if (g_buttonCallback) {
                        ButtonState state = BUTTON_PRESSED;
                        if (g_buttons[i].pressCount >= 2) {
                            state = BUTTON_DOUBLE_PRESS;
                            g_buttons[i].pressCount = 0;
                        }
                        g_buttonCallback((ButtonId)i, state);
                    }
                } else {
                    // Button released
                    g_buttons[i].releaseTime = now;

                    // Check if it was a long press
                    uint32_t holdTime = now - g_buttons[i].pressTime;
                    if (holdTime > LONG_PRESS_MS && g_buttonCallback) {
                        g_buttonCallback((ButtonId)i, BUTTON_LONG_PRESS);
                    }
                }
            }
        }
    }

    // Encoders would be processed here if connected to PCF8574 P4-P7
    // For now, encoders are not implemented
}

#endif // PLATFORM_ESP32
