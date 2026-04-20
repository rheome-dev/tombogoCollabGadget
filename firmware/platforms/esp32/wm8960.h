/**
 * WM8960 Audio Codec Driver Interface
 *
 * Header file for WM8960 headphone DAC on ESP32-S3.
 * Provides I2C control and I2S audio playback interface.
 *
 * Hardware: WM8960 Audio Board (I2C 0x1A)
 * I2S: GPIO16(WS), GPIO17(BCLK), GPIO18(DOUT) on I2S1
 *
 * Reference: https://www.waveshare.com/wiki/WM8960_Audio_Board
 */

#ifndef WM8960_H
#define WM8960_H

#include <stdint.h>
#include <stdbool.h>

// WM8960 I2C address (A0 pin grounded on Waveshare board)
#define WM8960_I2C_ADDR     0x1A

// I2S1 pin definitions (H2 header on Waveshare ESP32-S3)
#define WM8960_I2S_WS       16      // Word Select / LRCK
#define WM8960_I2S_BCLK     17      // Bit Clock
#define WM8960_I2S_DOUT     18      // Data Output (to DACDAT)
#define WM8960_I2S_PORT     I2S_NUM_1

// Sample rate (44.1kHz standard audio)
#define WM8960_SAMPLE_RATE     44100
#define WM8960_BITS_PER_SAMPLE 16
#define WM8960_CHANNELS        2

/**
 * Initialize WM8960 for headphone playback via I2S1.
 * Must be called after Wire.begin() and before I2S init.
 *
 * @return true if WM8960 is detected and initialized successfully
 */
bool WM8960_init(void);

/**
 * Check if WM8960 is connected on I2C bus.
 * @return true if device responds at address 0x1A
 */
bool WM8960_isConnected(void);

/**
 * Write a 16-bit register on WM8960 via I2C.
 * Uses WM8960 register write protocol: 7-bit reg addr + 9-bit data.
 *
 * @param reg Register number (0-63)
 * @param value 16-bit register value
 * @return true if I2C transaction succeeded
 */
bool WM8960_writeReg(uint8_t reg, uint16_t value);

/**
 * Read a 16-bit register from WM8960 via I2C.
 * Used for debugging - verify registers were written correctly.
 *
 * @param reg Register number (0-63)
 * @return 16-bit register value (0 if read failed)
 */
uint16_t WM8960_readReg(uint8_t reg);

/**
 * Set headphone output volume.
 * @param vol Volume level 0-255 (0 = mute, 255 = max, 127 = 0dB)
 */
void WM8960_setHeadphoneVolume(uint8_t vol);

/**
 * Mute or unmute headphone output.
 * @param mute true to mute, false to unmute
 */
void WM8960_setMute(bool mute);

/**
 * Configure WM8960 PLL for deriving internal clocks from BCLK.
 * Call this if using PLL mode instead of direct SYSCLK mode.
 *
 * @param sampleRate Target sample rate in Hz (e.g., 44100, 48000)
 * @return true if PLL configured successfully
 */
bool WM8960_configurePLL(uint32_t sampleRate);

#endif // WM8960_H
