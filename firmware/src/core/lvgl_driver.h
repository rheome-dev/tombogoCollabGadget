/**
 * LVGL Driver - Display and Input Integration
 *
 * Connects LVGL to the HAL display and touch interfaces
 */

#ifndef LVGL_DRIVER_H
#define LVGL_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize LVGL library and register display/input drivers
 * Must be called AFTER HAL_init() and DisplayHAL_init()
 * @return true if initialization successful
 */
bool LVGL_driver_init(void);

/**
 * LVGL tick handler - call from timer or loop
 * Should be called every 1-5ms
 * @param tick_ms Milliseconds since last call
 */
void LVGL_driver_tick(uint32_t tick_ms);

/**
 * LVGL task handler - call from main loop
 * Processes pending LVGL tasks (drawing, input, etc.)
 * @return Time in ms until next task needs to run
 */
uint32_t LVGL_driver_task(void);

/**
 * Check if LVGL is initialized and ready
 */
bool LVGL_driver_isReady(void);

/**
 * Get the default LVGL display object
 * @return lv_disp_t* (cast to void* for header portability)
 */
void* LVGL_driver_getDisplay(void);

/**
 * Get the default LVGL screen object
 * @return lv_obj_t* (cast to void* for header portability)
 */
void* LVGL_driver_getScreen(void);

#endif // LVGL_DRIVER_H
