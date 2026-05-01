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

// ─── Input Task ─────────────────────────────────────────────────────────────

static void inputTaskFunction(void* param) {
    (void)param;

    while (true) {
        const InputMsg* msg;
        while ((msg = MCPInput_poll()) != nullptr) {
            StageManager_handleInput(msg);
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
