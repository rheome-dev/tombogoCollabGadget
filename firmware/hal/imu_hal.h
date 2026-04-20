/**
 * Hardware Abstraction Layer - IMU (Inertial Measurement Unit) Interface
 *
 * Defines the contract for accelerometer/gyroscope hardware.
 */

#ifndef IMU_HAL_H
#define IMU_HAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * IMU Data structure
 */
struct IMUData {
    float ax, ay, az;    // Accelerometer (g)
    float gx, gy, gz;    // Gyroscope (deg/s)
    float pitch;         // Computed pitch (degrees)
    float roll;          // Computed roll (degrees)
};

/**
 * IMU HAL initialization
 */
void IMUHAL_init(void);

/**
 * Read IMU data
 * @param data Pointer to IMUData struct to fill
 * @return true if successful, false if failed
 */
bool IMUHAL_read(IMUData* data);

/**
 * Check if IMU is available
 */
bool IMUHAL_available(void);

#endif // IMU_HAL_H
