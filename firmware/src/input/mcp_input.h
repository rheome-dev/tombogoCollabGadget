/**
 * MCP23017 Input Driver
 *
 * Handles encoder, buttons, and joystick on the Waveshare board.
 * Uses Wire2 on GPIO 16/17, INTA on GPIO 18.
 * Interrupt-driven reading with software debounce.
 */

#ifndef MCP_INPUT_H
#define MCP_INPUT_H

#include <stdint.h>
#include <stdbool.h>

// ─── Event Types ────────────────────────────────────────────────────────────

typedef enum {
    EVT_NONE = 0,

    // Button events
    EVT_BUTTON_PRESS,        // Single press
    EVT_BUTTON_RELEASE,      // Release
    EVT_BUTTON_DOUBLE,       // Double-click within 300ms
    EVT_BUTTON_LONG,         // Held for 500ms

    // Encoder events
    EVT_ENCODER_CW,          // Clockwise click
    EVT_ENCODER_CCW,         // Counter-clockwise click
    EVT_ENCODER_PUSH,        // Encoder button press

    // Joystick events (5-way)
    EVT_JOY_UP,
    EVT_JOY_DOWN,
    EVT_JOY_LEFT,
    EVT_JOY_RIGHT,
    EVT_JOY_CENTER,
} InputEvent;

// ─── Input IDs ──────────────────────────────────────────────────────────────

typedef enum {
    INPUT_ENCODER_SW = 0,    // PA0
    INPUT_ENCODER_A  = 1,    // PA2
    INPUT_ENCODER_B  = 2,    // PA1
    INPUT_BTN_1      = 3,    // PA3 - Capture / top
    INPUT_BTN_2      = 4,    // PA4 - Shift / middle
    INPUT_BTN_3      = 5,    // PA5 - Btn3 / bottom
    INPUT_JOY_UP     = 11,   // PB3
    INPUT_JOY_LEFT   = 12,   // PB4
    INPUT_JOY_DOWN   = 13,   // PB5
    INPUT_JOY_CENTER = 14,   // PB6
    INPUT_JOY_RIGHT  = 15,   // PB7
    INPUT_COUNT
} InputId;

// ─── Event Structure ─────────────────────────────────────────────────────────

typedef struct {
    InputEvent type;
    uint8_t    id;
    uint32_t   timestamp;    // millis() at event time
} InputMsg;

// ─── API ────────────────────────────────────────────────────────────────────

/**
 * Initialize MCP23017 on Wire2 (GPIO 16/17).
 * Configures all pins as inputs with pull-ups.
 * Sets up INTA on GPIO 18 (active low).
 * Returns true on success.
 */
bool MCPInput_init(void);

/**
 * Poll MCP23017 for input changes.
 * Call this from the input task loop.
 * Returns the next queued event, or NULL if none.
 * Caller must NOT free the returned pointer.
 */
const InputMsg* MCPInput_poll(void);

/**
 * Check if the MCP23017 interrupt flag is set.
 * INTA pin goes low on any pin change.
 */
bool MCPInput_hasInterrupt(void);

/**
 * Get current shift modifier state.
 * True if shift button (PA4) is held.
 */
bool MCPInput_isShiftHeld(void);

/**
 * Get current encoder position delta since last call.
 * Positive = CW, negative = CCW.
 */
int8_t MCPInput_getEncoderDelta(void);

/**
 * Read raw GPIO state directly (for debug/diagnostics).
 * Returns true on success.
 */
bool MCPInput_getRawGPIO(uint8_t* gpioA, uint8_t* gpioB);

#endif // MCP_INPUT_H