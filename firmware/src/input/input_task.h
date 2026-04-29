/**
 * Input Task - FreeRTOS task for MCP23017 polling
 *
 * Runs on Core 0 at 10ms intervals.
 * Polls MCP23017, debounces, decodes encoder, fires events.
 */

#ifndef INPUT_TASK_H
#define INPUT_TASK_H

#include <stdint.h>

void InputTask_start(void);
bool InputTask_isShiftHeld(void);

#endif // INPUT_TASK_H
