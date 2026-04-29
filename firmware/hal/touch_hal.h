/**
 * Hardware Abstraction Layer - Touch Interface
 *
 * Defines the contract for touchscreen input.
 */

#ifndef TOUCH_HAL_H
#define TOUCH_HAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Touch Point structure
 */
struct TouchPoint {
    int16_t x;
    int16_t y;
    bool pressed;
};

/**
 * Touch HAL initialization
 */
void TouchHAL_init(void);

/**
 * Get number of touch points currently detected
 * @return Number of touch points (0-5 typically)
 */
uint8_t TouchHAL_getCount(void);

/**
 * Get touch point at index
 * @param index Index of touch point (0-based)
 * @param point Pointer to TouchPoint struct to fill
 * @return true if valid point, false if index out of range
 */
bool TouchHAL_getPoint(uint8_t index, TouchPoint* point);

/**
 * Check if touch is currently active
 */
bool TouchHAL_isPressed(void);

/**
 * Check if touch controller is initialized and ready
 */
bool TouchHAL_isReady(void);

/**
 * Raw read of touch count (no state caching) — for debug only
 */
uint8_t TouchHAL_getCount_raw(void);

/**
 * Raw read of touch point (no state caching) — for debug only
 * Returns true if valid point was read into *x, *y
 */
bool TouchHAL_getPoint_raw(uint8_t index, int16_t* x, int16_t* y);

#endif // TOUCH_HAL_H
