/**
 * UI Manager
 *
 * Handles display rendering and touch interaction using LVGL
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "../../hal/imu_hal.h"

/**
 * Initialize UI Manager
 * Must be called AFTER HAL_init()
 */
void UIManager_init(void);

/**
 * Update UI (call from main loop)
 * Processes LVGL tasks and input events
 */
void UIManager_update(void);

/**
 * Handle IMU data for UI
 * Can be used for tilt-based controls or visualizations
 */
void UIManager_handleIMU(IMUData imuData);

/**
 * Set status text on main screen
 * @param status Text to display
 */
void UIManager_setStatus(const char* status);

#endif // UI_MANAGER_H
