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

    // Set the interrupt pin so getDataReady() can use it
    qmi8658->setPins(QMI_INT2);

    // Scan I2C bus first to see what's there
    Serial.println("  [IMU] Scanning I2C bus for QMI8658...");
    for (uint8_t addr = 0x68; addr <= 0x6C; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        const char* found = (err == 0) ? "FOUND" : "no ack";
        Serial.printf("    0x%02X: %s\n", addr, found);
    }

    // Try low address (0x6B) first
    Serial.println("  [IMU] Trying QMI8658_L_SLAVE_ADDRESS (0x6B)...");
    if (!qmi8658->begin(Wire, 0x6B)) {
        Serial.println("  [IMU] 0x6B failed, trying 0x6A...");
        if (!qmi8658->begin(Wire, 0x6A)) {
            Serial.println("ESP32: QMI8658 init failed on both 0x6B and 0x6A!");
            imuAvailable = false;
            return;
        }
        Serial.println("  [IMU] Found at 0x6A!");
    }

    // Read chip ID for verification
    uint8_t chipId = qmi8658->whoAmI();
    Serial.printf("  [IMU] Chip ID: 0x%02X\n", chipId);

    // Configure accelerometer
    qmi8658->configAccelerometer(
        SensorQMI8658::ACC_RANGE_2G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_0
    );

    // Configure gyroscope
    qmi8658->configGyroscope(
        SensorQMI8658::GYR_RANGE_256DPS,
        SensorQMI8658::GYR_ODR_896_8Hz,
        SensorQMI8658::LPF_MODE_0
    );

    // CRITICAL: Enable the sensors (config alone doesn't start them)
    qmi8658->enableAccelerometer();
    qmi8658->enableGyroscope();

    // Enable data ready interrupt on INT2
    qmi8658->enableINT(SensorQMI8658::INTERRUPT_PIN_2);

    imuAvailable = true;
    lastIMUUpdate = millis();

    Serial.println("ESP32: IMU initialized successfully");
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

// Debug raw read — fills all 5 values, returns true on success
// Bypasses getDataReady() gate so we can see raw sensor state
bool IMUHAL_read_raw(float* ax, float* ay, float* az, float* pitch, float* roll) {
    if (!imuAvailable || !qmi8658) return false;

    // Read accel directly — no getDataReady() gate needed for debug
    float aax, aay, aaz;
    if (!qmi8658->getAccelerometer(aax, aay, aaz)) return false;

    if (ax) *ax = aax;
    if (ay) *ay = aay;
    if (az) *az = aaz;

    // Quick accel-based angles (no gyro integration for debug)
    if (pitch) *pitch = atan2(-aax, sqrt(aay * aay + aaz * aaz)) * 180.0f / PI;
    if (roll) *roll = atan2(aay, aaz) * 180.0f / PI;

    return true;
}

// Dump QMI8658 control registers for diagnostics
void IMUHAL_dumpRegs(void) {
    if (!imuAvailable || !qmi8658) return;
    qmi8658->dumpCtrlRegister();
    Serial.printf("  [IMU_REGS] update=0x%02X\n", qmi8658->update() & 0xFF);
}

#endif // PLATFORM_ESP32
