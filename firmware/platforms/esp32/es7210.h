/**
 * ES7210 - High Performance 4-Channel Audio ADC Driver
 *
 * Driver for Everest Semiconductor ES7210 microphone array ADC
 * Reference: https://github.com/espressif/esp-bsp/tree/master/components/es7210
 *
 * Hardware: Waveshare ESP32-S3-Touch-AMOLED-1.75
 * I2C Address: 0x40 (AD0=0, AD1=0)
 */

#ifndef ES7210_H
#define ES7210_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ES7210 I2C Address (7-bit, AD0=0, AD1=0)
// ============================================================================
#define ES7210_I2C_ADDR         0x40

// ============================================================================
// ES7210 Register Addresses
// ============================================================================
#define ES7210_RESET_REG00              0x00    // Reset control
#define ES7210_CLOCK_OFF_REG01          0x01    // Clock gating
#define ES7210_MAINCLK_REG02            0x02    // Main clock divider
#define ES7210_MASTER_CLK_REG03         0x03    // Master clock source
#define ES7210_LRCK_DIVH_REG04          0x04    // LRCK divider high byte
#define ES7210_LRCK_DIVL_REG05          0x05    // LRCK divider low byte
#define ES7210_POWER_DOWN_REG06         0x06    // Power down control
#define ES7210_OSR_REG07                0x07    // Oversampling rate
#define ES7210_MODE_CONFIG_REG08        0x08    // Master/slave mode
#define ES7210_TIME_CONTROL0_REG09      0x09    // Timing control 0
#define ES7210_TIME_CONTROL1_REG0A      0x0A    // Timing control 1
#define ES7210_SDP_INTERFACE1_REG11     0x11    // Serial data protocol 1
#define ES7210_SDP_INTERFACE2_REG12     0x12    // Serial data protocol 2 (TDM)
#define ES7210_ADC_AUTOMUTE_REG13       0x13    // Auto-mute control
#define ES7210_ADC34_MUTERANGE_REG14    0x14    // Mute range ADC3/4
#define ES7210_ADC12_MUTERANGE_REG15    0x15    // Mute range ADC1/2
#define ES7210_ADC34_HPF2_REG20         0x20    // HPF coefficient ADC3/4
#define ES7210_ADC34_HPF1_REG21         0x21    // HPF coefficient ADC3/4
#define ES7210_ADC12_HPF1_REG22         0x22    // HPF coefficient ADC1/2
#define ES7210_ADC12_HPF2_REG23         0x23    // HPF coefficient ADC1/2
#define ES7210_ANALOG_REG40             0x40    // Analog power control
#define ES7210_MIC12_BIAS_REG41         0x41    // MIC1/2 bias voltage
#define ES7210_MIC34_BIAS_REG42         0x42    // MIC3/4 bias voltage
#define ES7210_MIC1_GAIN_REG43          0x43    // MIC1 PGA gain
#define ES7210_MIC2_GAIN_REG44          0x44    // MIC2 PGA gain
#define ES7210_MIC3_GAIN_REG45          0x45    // MIC3 PGA gain
#define ES7210_MIC4_GAIN_REG46          0x46    // MIC4 PGA gain
#define ES7210_MIC1_POWER_REG47         0x47    // MIC1 power
#define ES7210_MIC2_POWER_REG48         0x48    // MIC2 power
#define ES7210_MIC3_POWER_REG49         0x49    // MIC3 power
#define ES7210_MIC4_POWER_REG4A         0x4A    // MIC4 power
#define ES7210_MIC12_POWER_REG4B        0x4B    // MIC1/2 bias & ADC power
#define ES7210_MIC34_POWER_REG4C        0x4C    // MIC3/4 bias & ADC power

// ============================================================================
// Microphone Selection Bitmask
// ============================================================================
typedef enum {
    ES7210_MIC_NONE = 0x00,
    ES7210_MIC1     = 0x01,
    ES7210_MIC2     = 0x02,
    ES7210_MIC3     = 0x04,
    ES7210_MIC4     = 0x08,
    ES7210_MIC_ALL  = 0x0F
} es7210_mic_select_t;

// ============================================================================
// Microphone Gain Values (0dB to 37.5dB)
// ============================================================================
typedef enum {
    ES7210_GAIN_0DB    = 0,
    ES7210_GAIN_3DB    = 1,
    ES7210_GAIN_6DB    = 2,
    ES7210_GAIN_9DB    = 3,
    ES7210_GAIN_12DB   = 4,
    ES7210_GAIN_15DB   = 5,
    ES7210_GAIN_18DB   = 6,
    ES7210_GAIN_21DB   = 7,
    ES7210_GAIN_24DB   = 8,
    ES7210_GAIN_27DB   = 9,
    ES7210_GAIN_30DB   = 10,
    ES7210_GAIN_33DB   = 11,
    ES7210_GAIN_34_5DB = 12,
    ES7210_GAIN_36DB   = 13,
    ES7210_GAIN_37_5DB = 14
} es7210_gain_t;

// ============================================================================
// I2S Data Format
// ============================================================================
typedef enum {
    ES7210_I2S_NORMAL = 0,      // Standard I2S
    ES7210_I2S_LEFT   = 1,      // Left-justified
    ES7210_I2S_RIGHT  = 2,      // Right-justified
    ES7210_I2S_DSP_A  = 3,      // DSP/PCM mode A (1-bit delay)
    ES7210_I2S_DSP_B  = 4       // DSP/PCM mode B (no delay)
} es7210_i2s_fmt_t;

// ============================================================================
// Bit Width
// ============================================================================
typedef enum {
    ES7210_BITS_16 = 16,
    ES7210_BITS_24 = 24,
    ES7210_BITS_32 = 32
} es7210_bits_t;

// ============================================================================
// Sample Rate
// ============================================================================
typedef enum {
    ES7210_RATE_8K   = 8000,
    ES7210_RATE_12K  = 12000,
    ES7210_RATE_16K  = 16000,
    ES7210_RATE_22K  = 22050,
    ES7210_RATE_24K  = 24000,
    ES7210_RATE_32K  = 32000,
    ES7210_RATE_44K  = 44100,
    ES7210_RATE_48K  = 48000,
    ES7210_RATE_96K  = 96000
} es7210_sample_rate_t;

// ============================================================================
// Configuration Structure
// ============================================================================
typedef struct {
    es7210_mic_select_t mics;           // Which microphones to enable
    es7210_gain_t       gain;           // PGA gain for all enabled mics
    es7210_i2s_fmt_t    i2s_format;     // I2S data format
    es7210_bits_t       bits;           // Bit depth (16/24/32)
    es7210_sample_rate_t sample_rate;   // Sample rate
    bool                enable_tdm;     // Enable TDM mode for 4ch
    uint32_t            mclk_freq;      // MCLK frequency in Hz
} es7210_config_t;

// Default configuration
#define ES7210_CONFIG_DEFAULT() {                                   \
    .mics = (es7210_mic_select_t)(ES7210_MIC1 | ES7210_MIC2),       \
    .gain = ES7210_GAIN_24DB,                                       \
    .i2s_format = ES7210_I2S_NORMAL,                                \
    .bits = ES7210_BITS_16,                                         \
    .sample_rate = ES7210_RATE_16K,                                 \
    .enable_tdm = false,                                            \
    .mclk_freq = 12288000                                           \
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize ES7210 with configuration
 * @param config Configuration structure
 * @return true on success, false on failure
 */
bool ES7210_init(const es7210_config_t* config);

/**
 * Deinitialize ES7210
 */
void ES7210_deinit(void);

/**
 * Start ADC conversion (enable microphones)
 * @return true on success
 */
bool ES7210_start(void);

/**
 * Stop ADC conversion (mute microphones)
 * @return true on success
 */
bool ES7210_stop(void);

/**
 * Set microphone gain
 * @param mics Bitmask of microphones to configure
 * @param gain Gain value
 * @return true on success
 */
bool ES7210_setGain(es7210_mic_select_t mics, es7210_gain_t gain);

/**
 * Set mute state
 * @param mute true to mute, false to unmute
 * @return true on success
 */
bool ES7210_setMute(bool mute);

/**
 * Read register value (for debugging)
 * @param reg Register address
 * @return Register value
 */
uint8_t ES7210_readReg(uint8_t reg);

/**
 * Check if ES7210 is present on I2C bus
 * @return true if device responds
 */
bool ES7210_isPresent(void);

/**
 * Dump all registers to serial (debug)
 */
void ES7210_dumpRegisters(void);

#endif // ES7210_H
