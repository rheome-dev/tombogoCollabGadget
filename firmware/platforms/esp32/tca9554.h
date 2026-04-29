/**
 * TCA9554 I2C GPIO Expander Driver
 *
 * Controls LCD power, LCD reset, and touch reset on Waveshare ESP32-S3-Touch-AMOLED-1.75
 * Reference: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
 */

#ifndef TCA9554_H
#define TCA9554_H

#include <stdint.h>
#include <stdbool.h>

// TCA9554 I2C address (A0=0, A1=0, A2=0 → 0x20 per datasheet)
// Was incorrectly 0x18 which collides with ES8311 codec
#define TCA9554_I2C_ADDR    0x20

// TCA9554 Registers
#define TCA9554_REG_INPUT   0x00  // Input port register (read-only)
#define TCA9554_REG_OUTPUT  0x01  // Output port register
#define TCA9554_REG_INVERT  0x02  // Polarity inversion register
#define TCA9554_REG_CONFIG  0x03  // Configuration register (0=output, 1=input)

// Pin assignments on Waveshare board
#define TCA9554_PIN_TP_RST   0    // P0: Touch panel reset (active low)
#define TCA9554_PIN_LCD_RST  1    // P1: LCD reset (active low)
#define TCA9554_PIN_LCD_PWR  2    // P2: LCD power enable (active high)
#define TCA9554_PIN_TE       3    // P3: Tearing effect (input)
#define TCA9554_PIN_SD_CS    4    // P4: SD card chip select (directly wired to SD slot)
// P5-P7: Unused/NC

/**
 * Initialize TCA9554 GPIO expander
 * Must be called BEFORE display and touch initialization
 * @return true if device is detected and configured
 */
bool TCA9554_init(void);

/**
 * Check if TCA9554 is detected on I2C bus
 * @return true if device responds to address
 */
bool TCA9554_isConnected(void);

/**
 * Set pin direction
 * @param pin Pin number (0-7)
 * @param output true for output, false for input
 */
void TCA9554_pinMode(uint8_t pin, bool output);

/**
 * Write to output pin
 * @param pin Pin number (0-7)
 * @param high true for HIGH, false for LOW
 */
void TCA9554_digitalWrite(uint8_t pin, bool high);

/**
 * Read input pin state
 * @param pin Pin number (0-7)
 * @return true if HIGH, false if LOW
 */
bool TCA9554_digitalRead(uint8_t pin);

/**
 * Power on LCD display
 * Sets LCD_PWR high and performs reset sequence
 */
void TCA9554_powerOnLCD(void);

/**
 * Power off LCD display
 */
void TCA9554_powerOffLCD(void);

/**
 * Reset touch controller
 * Performs reset pulse on TP_RST
 */
void TCA9554_resetTouch(void);

/**
 * Get current output register state
 * @return 8-bit output register value
 */
uint8_t TCA9554_getOutputState(void);

#endif // TCA9554_H
