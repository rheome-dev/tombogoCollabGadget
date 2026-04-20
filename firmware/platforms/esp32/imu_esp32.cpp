/**
 * ESP32 Platform - IMU HAL Implementation
 *
 * Uses SensorLib for QMI8658 6-axis IMU
 * Reference: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
 */

#ifdef PLATFORM_ESP32

#include "../../hal/imu_hal.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <SensorQMI8658.hpp>

// IMU sensor object
static SensorQMI8658* qmi8658 = nullptr;

// IMU state
static bool imuAvailable = false;

// Complementary filter state for pitch/roll
static float pitch = 0;
static float roll = 0;
static uint32_t lastIMUUpdate = 0;

void IMUHAL_init(void) {
    Serial.println("ESP32: Initializing IMU HAL...");

    // Note: I2C is already initialized in HAL_init()

    // Create QMI8658 sensor instance
    qmi8658 = new SensorQMI8658();

    // Initialize the sensor
    if (!qmi8658->begin(Wire, QMI8658_ADDR)) {
        Serial.println("ESP32: QMI8658 init failed!");
        imuAvailable = false;
        return;
    }

    // Configure accelerometer
    // Range: ±2g, ±4g, ±8g, ±16g
    qmi8658->configAccelerometer(
        SensorQMI8658::ACC_RANGE_2G,       // Range
        SensorQMI8658::ACC_ODR_1000Hz,    // Output data rate
        SensorQMI8658::LPF_MODE_0         // Low-pass filter
    );

    // Configure gyroscope
    // Range: ±16, ±32, ±64, ±128, ±256, ±512, ±1024, ±2048 deg/s
    qmi8658->configGyroscope(
        SensorQMI8658::GYR_RANGE_256DPS,   // Range
        SensorQMI8658::GYR_ODR_896_8Hz,   // Output data rate
        SensorQMI8658::LPF_MODE_0         // Low-pass filter
    );

    // Enable interrupts (to signal data ready)
    qmi8658->enableINT(SensorQMI8658::INTERRUPT_PIN_1);

    imuAvailable = true;
    lastIMUUpdate = millis();

    Serial.println("ESP32: IMU initialized successfully");
    Serial.printf("  QMI8658 I2C Address: 0x%02X\n", QMI8658_ADDR);
}

bool IMUHAL_read(IMUData* data) {
    if (!imuAvailable || !qmi8658 || !data) {
        return false;
    }

    // Check if data is ready
    if (!qmi8658->getDataReady()) {
        return false;
    }

    // Get accelerometer and gyroscope data
    float ax, ay, az;
    float gx, gy, gz;

    if (!qmi8658->getAccelerometer(ax, ay, az)) {
        return false;
    }

    if (!qmi8658->getGyroscope(gx, gy, gz)) {
        return false;
    }

    // Fill in the IMU data structure
    data->ax = ax;
    data->ay = ay;
    data->az = az;

    data->gx = gx;
    data->gy = gy;
    data->gz = gz;

    // Calculate pitch and roll using accelerometer data
    // Simple complementary filter approach
    uint32_t now = millis();
    float dt = (now - lastIMUUpdate) / 1000.0f;
    lastIMUUpdate = now;

    // Calculate pitch and roll from accelerometer
    float accelPitch = atan2(-data->ax, sqrt(data->ay * data->ay + data->az * data->az)) * 180.0f / PI;
    float accelRoll = atan2(data->ay, data->az) * 180.0f / PI;

    // Complementary filter: combine gyro integration with accel angle
    float alpha = 0.98f;
    pitch = alpha * (pitch + data->gy * dt) + (1.0f - alpha) * accelPitch;
    roll = alpha * (roll + data->gx * dt) + (1.0f - alpha) * accelRoll;

    data->pitch = pitch;
    data->roll = roll;

    return true;
}

bool IMUHAL_available(void) {
    return imuAvailable;
}

#endif // PLATFORM_ESP32
