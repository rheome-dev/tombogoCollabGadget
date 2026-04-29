/**
 * Input Task Implementation
 *
 * Polls MCP23017 and routes events to stage manager.
 * No encoder accumulation here — events go directly through the queue.
 */

#include "input_task.h"
#include "mcp_input.h"
#include "../core/stage_manager.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static TaskHandle_t taskHandle = NULL;

// Raw input debug state (mirror of MCP state for printing)
static uint8_t lastDebugGpioA = 0xFF;
static uint8_t lastDebugGpioB = 0xFF;

// IMU debug
static bool imuAvailable = false;
static uint32_t imuPollCount = 0;

// Touch debug (used for released detection)
static bool lastTouchPressed = false;

// ─── Debug helpers ───────────────────────────────────────────────────────────

static const char* eventName(InputEvent type) {
    switch (type) {
        case EVT_NONE:           return "NONE";
        case EVT_BUTTON_PRESS:   return "BTN_PRESS";
        case EVT_BUTTON_RELEASE: return "BTN_RELEASE";
        case EVT_BUTTON_DOUBLE:  return "BTN_DOUBLE";
        case EVT_BUTTON_LONG:    return "BTN_LONG";
        case EVT_ENCODER_CW:     return "ENC_CW";
        case EVT_ENCODER_CCW:    return "ENC_CCW";
        case EVT_ENCODER_PUSH:   return "ENC_PUSH";
        case EVT_JOY_UP:         return "JOY_UP";
        case EVT_JOY_DOWN:       return "JOY_DOWN";
        case EVT_JOY_LEFT:       return "JOY_LEFT";
        case EVT_JOY_RIGHT:      return "JOY_RIGHT";
        case EVT_JOY_CENTER:     return "JOY_CENTER";
        default:                 return "UNKNOWN";
    }
}

static const char* inputName(uint8_t id) {
    switch (id) {
        case INPUT_ENCODER_SW:  return "ENC_SW";
        case INPUT_ENCODER_A:   return "ENC_A";
        case INPUT_ENCODER_B:   return "ENC_B";
        case INPUT_BTN_1:       return "BTN1";
        case INPUT_BTN_2:       return "BTN2";
        case INPUT_BTN_3:       return "BTN3";
        case INPUT_JOY_UP:      return "JOY_UP";
        case INPUT_JOY_LEFT:    return "JOY_LEFT";
        case INPUT_JOY_DOWN:    return "JOY_DOWN";
        case INPUT_JOY_CENTER:  return "JOY_C";
        case INPUT_JOY_RIGHT:   return "JOY_RIGHT";
        default:                return "UNK";
    }
}

// ─── Input Task ─────────────────────────────────────────────────────────────

static void inputTaskFunction(void* param) {
    (void)param;
    Serial.println("InputTask: Started on Core 0");

    // Check IMU availability
    extern bool IMUHAL_available(void);
    imuAvailable = IMUHAL_available();

    uint32_t debugPrintMs = 0;

    while (true) {
        const InputMsg* msg;

        // Process all queued MCP/encoder/button/joystick events
        while ((msg = MCPInput_poll()) != nullptr) {
            StageManager_handleInput(msg);

            // Print every input event to serial
            Serial.printf("[INPUT] %-12s %s\n", eventName(msg->type), inputName(msg->id));
        }

        // Periodic raw debug: print raw GPIO if changed (every 2s or on change)
        uint32_t now = millis();
        if (now - debugPrintMs > 2000) {
            debugPrintMs = now;

            // Read raw MCP state
            uint8_t gpioA, gpioB;
            if (MCPInput_getRawGPIO(&gpioA, &gpioB)) {
                Serial.printf("[RAW_GPIO] A=0x%02X B=0x%02X | Shift=%s\n",
                    gpioA, gpioB,
                    MCPInput_isShiftHeld() ? "ON" : "OFF");
            }

            // Print IMU state every 2s
            extern bool IMUHAL_read_raw(float* ax, float* ay, float* az, float* pitch, float* roll);
            float ax, ay, az, pitch, roll;
            if (IMUHAL_read_raw(&ax, &ay, &az, &pitch, &roll)) {
                Serial.printf("[IMU] pitch=%.1f roll=%.1f acc=(%.2f, %.2f, %.2f)\n",
                    pitch, roll, ax, ay, az);
            } else {
                Serial.printf("[IMU] poll=%u available=%d\n", imuPollCount, imuAvailable);
                // Dump register state on failure so we can see why
                extern void IMUHAL_dumpRegs(void);
                IMUHAL_dumpRegs();
            }
            imuPollCount = 0;

            // Print touch state
            extern bool TouchHAL_isReady(void);
            extern uint8_t TouchHAL_getCount_raw(void);
            extern bool TouchHAL_getPoint_raw(uint8_t idx, int16_t* x, int16_t* y);

            if (TouchHAL_isReady()) {
                uint8_t count = TouchHAL_getCount_raw();
                if (count > 0) {
                    int16_t tx, ty;
                    if (TouchHAL_getPoint_raw(0, &tx, &ty)) {
                        Serial.printf("[TOUCH] count=%u x=%d y=%d\n", count, tx, ty);
                    }
                }
            }
        } else {
            // Count IMU polls between prints
            extern bool IMUHAL_read_raw(float* ax, float* ay, float* az, float* pitch, float* roll);
            if (imuAvailable) {
                float ax, ay, az, pitch, roll;
                if (IMUHAL_read_raw(&ax, &ay, &az, &pitch, &roll)) {
                    imuPollCount++;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void InputTask_start(void) {
    if (!MCPInput_init()) {
        Serial.println("InputTask: MCPInput_init failed - task not started");
        return;
    }

    xTaskCreatePinnedToCore(
        inputTaskFunction,
        "InputTask",
        4096,
        nullptr,
        5,
        &taskHandle,
        0  // Core 0
    );

    Serial.println("InputTask: Started (Core 0, 10ms poll)");
}

bool InputTask_isShiftHeld(void) {
    return MCPInput_isShiftHeld();
}
