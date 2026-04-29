/**
 * Stage Manager - State machine for looper UI flow
 *
 * Manages transitions between IDLE → CAPTURE_REVIEW → CHOP → RESONATE
 * based on physical input events.
 */

#ifndef STAGE_MANAGER_H
#define STAGE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "../input/mcp_input.h"

// ─── Stage Definitions ───────────────────────────────────────────────────────

typedef enum {
    STAGE_IDLE = 0,
    STAGE_CAPTURE_REVIEW,
    STAGE_CHOP,
    STAGE_RESONATE,
    STAGE_COUNT
} Stage;

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * Initialize the stage manager.
 */
void StageManager_init(void);

/**
 * Get the current stage.
 */
Stage StageManager_current(void);

/**
 * Get the current stage name for display.
 */
const char* StageManager_stageName(void);

/**
 * Handle an input event.
 * Called from the input task or main loop.
 */
void StageManager_handleInput(const InputMsg* msg);

/**
 * Called each main loop tick.
 * Updates stage-specific animations and timers.
 */
void StageManager_update(void);

/**
 * Force transition to a specific stage.
 * Use with care - bypasses normal transition rules.
 */
void StageManager_goto(Stage stage);

#endif // STAGE_MANAGER_H