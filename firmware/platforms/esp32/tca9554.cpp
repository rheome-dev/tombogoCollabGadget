/**
 * TCA9554 I2C GPIO Expander Driver Implementation
 *
 * Controls LCD power, LCD reset, and touch reset on Waveshare ESP32-S3-Touch-AMOLED-1.75
 */

#ifdef PLATFORM_ESP32

#include "tca9554.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>

// Internal state tracking
static uint8_t outputState = 0xFF;  // Default all high (inactive)
static uint8_t configState = 0xFF;  // Default all inputs
static bool initialized = false;

// Write to TCA9554 register
static bool tca9554_writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(TCA9554_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

// Read from TCA9554 register
static uint8_t tca9554_readReg(uint8_t reg) {
    Wire.beginTransmission(TCA9554_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)TCA9554_I2C_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}

bool TCA9554_init(void) {
    Serial.println("ESP32: Initializing TCA9554 IO expander...");

    // Check if device is present
    Wire.beginTransmission(TCA9554_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("ESP32: TCA9554 not found at address 0x18!");
        return false;
    }

    // Configure pins:
    // P0 (TP_RST)  - Output, start HIGH (not reset)
    // P1 (LCD_RST) - Output, start HIGH (not reset)
    // P2 (LCD_PWR) - Output, start LOW (power off initially)
    // P3 (TE)      - Input
    // P4 (SD_CS)   - Output, start HIGH (not selected)
    // P5-P7        - Input (unused)

    // Set output state before configuring as outputs
    // Bits: 7654 3210
    //       1111 1011 = 0xFB (LCD_PWR low, others high)
    outputState = 0xFB;
    if (!tca9554_writeReg(TCA9554_REG_OUTPUT, outputState)) {
        Serial.println("ESP32: Failed to write TCA9554 output register");
        return false;
    }

    // Set pin directions (0=output, 1=input)
    // P0-P2, P4 as outputs, P3, P5-P7 as inputs
    // Bits: 7654 3210
    //       1110 1000 = 0xE8
    configState = 0xE8;
    if (!tca9554_writeReg(TCA9554_REG_CONFIG, configState)) {
        Serial.println("ESP32: Failed to write TCA9554 config register");
        return false;
    }

    // No polarity inversion needed
    tca9554_writeReg(TCA9554_REG_INVERT, 0x00);

    initialized = true;
    Serial.println("ESP32: TCA9554 initialized successfully");
    return true;
}

bool TCA9554_isConnected(void) {
    Wire.beginTransmission(TCA9554_I2C_ADDR);
    return Wire.endTransmission() == 0;
}

void TCA9554_pinMode(uint8_t pin, bool output) {
    if (pin > 7) return;

    if (output) {
        configState &= ~(1 << pin);  // Clear bit = output
    } else {
        configState |= (1 << pin);   // Set bit = input
    }

    tca9554_writeReg(TCA9554_REG_CONFIG, configState);
}

void TCA9554_digitalWrite(uint8_t pin, bool high) {
    if (pin > 7) return;

    if (high) {
        outputState |= (1 << pin);   // Set bit = high
    } else {
        outputState &= ~(1 << pin);  // Clear bit = low
    }

    tca9554_writeReg(TCA9554_REG_OUTPUT, outputState);
}

bool TCA9554_digitalRead(uint8_t pin) {
    if (pin > 7) return false;

    uint8_t input = tca9554_readReg(TCA9554_REG_INPUT);
    return (input & (1 << pin)) != 0;
}

void TCA9554_powerOnLCD(void) {
    Serial.println("ESP32: Powering on LCD via TCA9554...");

    // Step 1: Assert reset (active low)
    TCA9554_digitalWrite(TCA9554_PIN_LCD_RST, false);
    delay(10);

    // Step 2: Enable power
    TCA9554_digitalWrite(TCA9554_PIN_LCD_PWR, true);
    delay(50);  // Wait for power to stabilize

    // Step 3: Release reset
    TCA9554_digitalWrite(TCA9554_PIN_LCD_RST, true);
    delay(50);  // Wait for LCD to initialize

    Serial.println("ESP32: LCD power on sequence complete");
}

void TCA9554_powerOffLCD(void) {
    Serial.println("ESP32: Powering off LCD...");

    // Assert reset before power off
    TCA9554_digitalWrite(TCA9554_PIN_LCD_RST, false);
    delay(10);

    // Disable power
    TCA9554_digitalWrite(TCA9554_PIN_LCD_PWR, false);

    Serial.println("ESP32: LCD powered off");
}

void TCA9554_resetTouch(void) {
    Serial.println("ESP32: Resetting touch controller...");

    // Assert reset (active low)
    TCA9554_digitalWrite(TCA9554_PIN_TP_RST, false);
    delay(10);

    // Release reset
    TCA9554_digitalWrite(TCA9554_PIN_TP_RST, true);
    delay(50);  // Wait for touch controller to initialize

    Serial.println("ESP32: Touch reset complete");
}

uint8_t TCA9554_getOutputState(void) {
    return outputState;
}

#endif // PLATFORM_ESP32
