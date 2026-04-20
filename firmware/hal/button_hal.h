/**
 * Hardware Abstraction Layer - Button/Encoder Interface
 *
 * Defines the contract for physical button and rotary encoder inputs.
 * Supports PCF8574 GPIO expander for button reading.
 */

#ifndef BUTTON_HAL_H
#define BUTTON_HAL_H

#include <stdint.h>
#include <stdbool.h>

// Button IDs
typedef enum {
    BUTTON_RECORD = 0,
    BUTTON_STOP = 1,
    BUTTON_LOOP_SELECT = 2,
    BUTTON_EFFECT_BYPASS = 3,
    BUTTON_COUNT = 4
} ButtonId;

// Button states
typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED = 1,
    BUTTON_LONG_PRESS = 2,
    BUTTON_DOUBLE_PRESS = 3
} ButtonState;

// Rotary encoder IDs
typedef enum {
    ENCODER_VOLUME = 0,
    ENCODER_PARAM = 1,
    ENCODER_COUNT = 2
} EncoderId;

/**
 * Button event callback
 */
typedef void (*ButtonCallback)(ButtonId id, ButtonState state);

/**
 * Encoder event callback (delta: +1 or -1)
 */
typedef void (*EncoderCallback)(EncoderId id, int8_t delta);

/**
 * Initialize button/encoder HAL
 */
void ButtonHAL_init(void);

/**
 * Check if a button is currently pressed
 * @param id Button ID
 * @return true if pressed (active low)
 */
bool ButtonHAL_isPressed(ButtonId id);

/**
 * Get button state (with debounce)
 * @param id Button ID
 * @return Current button state
 */
ButtonState ButtonHAL_getState(ButtonId id);

/**
 * Get encoder position
 * @param id Encoder ID
 * @return Current encoder position (accumulative)
 */
int32_t ButtonHAL_getEncoder(EncoderId id);

/**
 * Reset encoder position to zero
 * @param id Encoder ID
 */
void ButtonHAL_resetEncoder(EncoderId id);

/**
 * Register button press callback
 */
void ButtonHAL_onPress(ButtonCallback callback);

/**
 * Register encoder change callback
 */
void ButtonHAL_onEncoderChange(EncoderCallback callback);

/**
 * Process button/encoder inputs (call from loop)
 */
void ButtonHAL_update(void);

/**
 * Check if PCF8574 is connected
 * @return true if device responds
 */
bool ButtonHAL_isConnected(void);

#endif // BUTTON_HAL_H
