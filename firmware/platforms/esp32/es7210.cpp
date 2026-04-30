/**
 * ES7210 - High Performance 4-Channel Audio ADC Driver
 *
 * Implementation for Waveshare ESP32-S3-Touch-AMOLED-1.75
 * Reference: Espressif ESP-BSP es7210 component
 * https://github.com/espressif/esp-bsp/tree/master/components/es7210
 */

#ifdef PLATFORM_ESP32

#include "es7210.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>

// ============================================================================
// Module State
// ============================================================================
static bool es7210_initialized = false;
static es7210_config_t current_config;

// ============================================================================
// Clock Coefficient Table
// Maps MCLK/sample_rate combinations to register settings
// Format: {mclk, rate, osr, adc_div, dll_bypass, doubler, lrck_h, lrck_l}
// ============================================================================
typedef struct {
    uint32_t mclk;
    uint32_t rate;
    uint8_t  osr;
    uint8_t  adc_div;
    uint8_t  dll_bypass;
    uint8_t  doubler;
    uint8_t  lrck_h;
    uint8_t  lrck_l;
} es7210_coeff_t;

static const es7210_coeff_t es7210_coeff_div[] = {
    // mclk       rate   osr   div  bypass doubler lrck_h lrck_l
    {12288000,   8000,  0x06, 0x01, 0x00,  0x00,   0x06,  0x00},  // 12.288MHz / 8kHz
    {12288000,  12000,  0x06, 0x01, 0x00,  0x00,   0x04,  0x00},  // 12.288MHz / 12kHz
    {12288000,  16000,  0x06, 0x01, 0x00,  0x00,   0x03,  0x00},  // 12.288MHz / 16kHz
    {12288000,  24000,  0x06, 0x01, 0x00,  0x00,   0x02,  0x00},  // 12.288MHz / 24kHz
    {12288000,  32000,  0x06, 0x01, 0x00,  0x00,   0x01,  0x80},  // 12.288MHz / 32kHz
    {12288000,  48000,  0x06, 0x01, 0x00,  0x00,   0x01,  0x00},  // 12.288MHz / 48kHz
    {12288000,  96000,  0x02, 0x00, 0x00,  0x00,   0x00,  0x80},  // 12.288MHz / 96kHz

    {11289600,   8000,  0x06, 0x01, 0x00,  0x00,   0x05,  0x82},  // 11.2896MHz / 8kHz
    {11289600,  11025,  0x06, 0x01, 0x00,  0x00,   0x04,  0x00},  // 11.2896MHz / 11.025kHz
    {11289600,  22050,  0x06, 0x01, 0x00,  0x00,   0x02,  0x00},  // 11.2896MHz / 22.05kHz
    {11289600,  44100,  0x06, 0x01, 0x00,  0x00,   0x01,  0x00},  // 11.2896MHz / 44.1kHz

    // Lower MCLK rates (using clock doubler)
    {4096000,    8000,  0x06, 0x01, 0x01,  0x01,   0x02,  0x00},  // 4.096MHz / 8kHz
    {4096000,   16000,  0x06, 0x01, 0x01,  0x01,   0x01,  0x00},  // 4.096MHz / 16kHz
    {4096000,   32000,  0x02, 0x00, 0x00,  0x00,   0x00,  0x80},  // 4.096MHz / 32kHz

    // Fallback entry (use DLL)
    {0,             0,  0x06, 0x01, 0x00,  0x00,   0x03,  0x00},
};

// ============================================================================
// Low-Level I2C Functions
// ============================================================================

static bool es7210_write_reg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(ES7210_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static uint8_t es7210_read_reg(uint8_t reg) {
    Wire.beginTransmission(ES7210_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0xFF;
    }
    Wire.requestFrom((uint8_t)ES7210_I2C_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}

static bool es7210_update_reg(uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t current = es7210_read_reg(reg);
    uint8_t updated = (current & ~mask) | (value & mask);
    return es7210_write_reg(reg, updated);
}

// ============================================================================
// Find Clock Coefficients
// ============================================================================
static const es7210_coeff_t* es7210_find_coeff(uint32_t mclk, uint32_t rate) {
    for (size_t i = 0; i < sizeof(es7210_coeff_div) / sizeof(es7210_coeff_div[0]); i++) {
        if (es7210_coeff_div[i].mclk == mclk && es7210_coeff_div[i].rate == rate) {
            return &es7210_coeff_div[i];
        }
    }
    // Return last entry as fallback
    return &es7210_coeff_div[sizeof(es7210_coeff_div) / sizeof(es7210_coeff_div[0]) - 1];
}

// ============================================================================
// Configure Clock
// ============================================================================
static bool es7210_config_clock(const es7210_config_t* config) {
    const es7210_coeff_t* coeff = es7210_find_coeff(config->mclk_freq, config->sample_rate);

    Serial.printf("ES7210: Clock config for MCLK=%d, rate=%d\n",
                  config->mclk_freq, config->sample_rate);

    // OSR configuration
    es7210_write_reg(ES7210_OSR_REG07, coeff->osr);

    // Main clock divider
    es7210_write_reg(ES7210_MAINCLK_REG02, coeff->adc_div);

    // Master clock source (slave mode, MCLK from pin)
    uint8_t clk_src = 0x00;  // Slave mode
    if (coeff->doubler) {
        clk_src |= 0x01;  // Enable clock doubler
    }
    es7210_write_reg(ES7210_MASTER_CLK_REG03, clk_src);

    // LRCK divider
    es7210_write_reg(ES7210_LRCK_DIVH_REG04, coeff->lrck_h);
    es7210_write_reg(ES7210_LRCK_DIVL_REG05, coeff->lrck_l);

    // Power down DLL if bypass enabled
    if (coeff->dll_bypass) {
        es7210_update_reg(ES7210_POWER_DOWN_REG06, 0x04, 0x04);
    } else {
        es7210_update_reg(ES7210_POWER_DOWN_REG06, 0x04, 0x00);
    }

    return true;
}

// ============================================================================
// Configure I2S Format
// ============================================================================
static bool es7210_config_i2s(const es7210_config_t* config) {
    uint8_t sdp1 = 0x00;

    // Bit width — word length is in bits [7:5]
    switch (config->bits) {
        case ES7210_BITS_16:
            sdp1 |= 0x60;  // bits[7:5] = 011 → 16-bit
            break;
        case ES7210_BITS_24:
            sdp1 |= 0x00;  // bits[7:5] = 000 → 24-bit
            break;
        case ES7210_BITS_32:
            sdp1 |= 0x80;  // bits[7:5] = 100 → 32-bit
            break;
        default:
            Serial.printf("ES7210: WARNING — unknown bits value %d, forcing 16-bit\n", (int)config->bits);
            sdp1 |= 0x60;
            break;
    }

    // Format
    switch (config->i2s_format) {
        case ES7210_I2S_NORMAL:
            sdp1 |= 0x00;
            break;
        case ES7210_I2S_LEFT:
            sdp1 |= 0x01;
            break;
        case ES7210_I2S_RIGHT:
            sdp1 |= 0x02;
            break;
        case ES7210_I2S_DSP_A:
        case ES7210_I2S_DSP_B:
            sdp1 |= 0x03;  // DSP mode
            break;
    }

    Serial.printf("ES7210: Writing SDP1 REG[0x11] = 0x%02X (bits=%d, fmt=%d)\n",
                  sdp1, (int)config->bits, (int)config->i2s_format);

    // Write and verify — this register is critical for data alignment
    es7210_write_reg(ES7210_SDP_INTERFACE1_REG11, sdp1);
    delay(5);
    uint8_t readback = es7210_read_reg(ES7210_SDP_INTERFACE1_REG11);
    if (readback != sdp1) {
        Serial.printf("ES7210: SDP1 write FAILED! Wrote 0x%02X, read back 0x%02X — retrying\n",
                      sdp1, readback);
        // Retry
        es7210_write_reg(ES7210_SDP_INTERFACE1_REG11, sdp1);
        delay(5);
        readback = es7210_read_reg(ES7210_SDP_INTERFACE1_REG11);
        Serial.printf("ES7210: SDP1 retry result: 0x%02X (expected 0x%02X)\n",
                      readback, sdp1);
    } else {
        Serial.printf("ES7210: SDP1 verified OK: 0x%02X\n", readback);
    }

    // TDM configuration
    uint8_t sdp2 = 0x00;
    if (config->enable_tdm) {
        sdp2 |= 0x02;  // TDM enable with 4 slots
        Serial.println("ES7210: TDM mode enabled (4 channels)");
    } else {
        sdp2 = 0x00;
        Serial.println("ES7210: Standard I2S mode (2 channels)");
    }
    es7210_write_reg(ES7210_SDP_INTERFACE2_REG12, sdp2);

    return true;
}

// ============================================================================
// Configure Microphones
// ============================================================================
static bool es7210_config_mics(const es7210_config_t* config) {
    uint8_t mics = config->mics;

    Serial.printf("ES7210: Enabling mics: 0x%02X\n", mics);

    // Microphone bias configuration (2.87V typical for MEMS mics)
    if (mics & (ES7210_MIC1 | ES7210_MIC2)) {
        es7210_write_reg(ES7210_MIC12_BIAS_REG41, 0x70);  // MIC1/2 bias enable
    }
    if (mics & (ES7210_MIC3 | ES7210_MIC4)) {
        es7210_write_reg(ES7210_MIC34_BIAS_REG42, 0x70);  // MIC3/4 bias enable
    }

    // Individual microphone power control
    es7210_write_reg(ES7210_MIC1_POWER_REG47, (mics & ES7210_MIC1) ? 0x08 : 0x00);
    es7210_write_reg(ES7210_MIC2_POWER_REG48, (mics & ES7210_MIC2) ? 0x08 : 0x00);
    es7210_write_reg(ES7210_MIC3_POWER_REG49, (mics & ES7210_MIC3) ? 0x08 : 0x00);
    es7210_write_reg(ES7210_MIC4_POWER_REG4A, (mics & ES7210_MIC4) ? 0x08 : 0x00);

    // MIC1/2 power block (bias, ADC, PGA)
    uint8_t mic12_pwr = 0x00;
    if (mics & ES7210_MIC1) mic12_pwr |= 0x01;  // ADC1 power
    if (mics & ES7210_MIC2) mic12_pwr |= 0x02;  // ADC2 power
    if (mics & (ES7210_MIC1 | ES7210_MIC2)) {
        mic12_pwr |= 0x0C;  // PGA power for MIC1/2
    }
    es7210_write_reg(ES7210_MIC12_POWER_REG4B, mic12_pwr);

    // MIC3/4 power block
    uint8_t mic34_pwr = 0x00;
    if (mics & ES7210_MIC3) mic34_pwr |= 0x01;  // ADC3 power
    if (mics & ES7210_MIC4) mic34_pwr |= 0x02;  // ADC4 power
    if (mics & (ES7210_MIC3 | ES7210_MIC4)) {
        mic34_pwr |= 0x0C;  // PGA power for MIC3/4
    }
    es7210_write_reg(ES7210_MIC34_POWER_REG4C, mic34_pwr);

    // Set gain for enabled microphones
    ES7210_setGain(config->mics, config->gain);

    return true;
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool ES7210_isPresent(void) {
    Wire.beginTransmission(ES7210_I2C_ADDR);
    return Wire.endTransmission() == 0;
}

bool ES7210_init(const es7210_config_t* config) {
    Serial.println("ES7210: Initializing...");

    // Check device presence
    if (!ES7210_isPresent()) {
        Serial.println("ES7210: Device not found at I2C address 0x40!");
        return false;
    }
    Serial.println("ES7210: Device found");

    // Store config
    current_config = *config;

    // ========================================================================
    // 1) Software Reset (ESPHome reference: 0xFF then 0x32)
    // ========================================================================
    es7210_write_reg(ES7210_RESET_REG00, 0xFF);  // Full reset
    delay(20);
    es7210_write_reg(ES7210_RESET_REG00, 0x32);  // Exit reset to intermediate state
    delay(20);

    // ========================================================================
    // 2) Gate clocks initially (Espressif reference: 0x3F)
    // ========================================================================
    es7210_write_reg(ES7210_CLOCK_OFF_REG01, 0x3F);

    // ========================================================================
    // 3) Timing Configuration
    // ========================================================================
    es7210_write_reg(ES7210_TIME_CONTROL0_REG09, 0x30);
    es7210_write_reg(ES7210_TIME_CONTROL1_REG0A, 0x30);

    // ========================================================================
    // 4) High-Pass Filter (DC blocking for microphones)
    // ========================================================================
    es7210_write_reg(ES7210_ADC12_HPF2_REG23, 0x2A);
    es7210_write_reg(ES7210_ADC12_HPF1_REG22, 0x0A);
    es7210_write_reg(ES7210_ADC34_HPF2_REG20, 0x0A);
    es7210_write_reg(ES7210_ADC34_HPF1_REG21, 0x2A);

    // ========================================================================
    // 5) Mode Configuration (slave mode)
    // ========================================================================
    es7210_write_reg(ES7210_MODE_CONFIG_REG08, 0x00);

    // ========================================================================
    // 6) Analog Power (initial)
    // ========================================================================
    es7210_write_reg(ES7210_ANALOG_REG40, 0xC3);
    delay(10);

    // ========================================================================
    // 7) Mic bias only (power regs need clocks active)
    // ========================================================================
    uint8_t mics = config->mics;
    if (mics & (ES7210_MIC1 | ES7210_MIC2)) {
        es7210_write_reg(ES7210_MIC12_BIAS_REG41, 0x70);
    }
    if (mics & (ES7210_MIC3 | ES7210_MIC4)) {
        es7210_write_reg(ES7210_MIC34_BIAS_REG42, 0x70);
    }

    // ========================================================================
    // 8) Clock Configuration (ESPHome reference: write full coefficient table)
    // Previous approach skipped clock config in slave mode per Espressif
    // esp_codec_dev, but ESPHome's working driver writes all clock regs.
    // Values for {12288000, 16000}: adc_div=3, dll=1, doubler=1, osr=0x20
    // ========================================================================
    es7210_write_reg(ES7210_MASTER_CLK_REG03, 0x00);  // Slave mode
    es7210_write_reg(ES7210_MAINCLK_REG02, 0xC3);     // adc_div=3 | doubler | dll
    es7210_write_reg(ES7210_OSR_REG07, 0x20);          // OSR
    es7210_write_reg(ES7210_LRCK_DIVH_REG04, 0x03);   // LRCK divider high
    es7210_write_reg(ES7210_LRCK_DIVL_REG05, 0x00);   // LRCK divider low
    Serial.printf("ES7210: Clock config — REG02=0xC3 REG07=0x20 REG04=0x03 REG05=0x00\n");

    // ========================================================================
    // 9) I2S Interface Configuration (ESPHome does this before mic power)
    // ========================================================================
    if (!es7210_config_i2s(config)) {
        Serial.println("ES7210: I2S config failed");
        return false;
    }

    // ========================================================================
    // 10) Mic gain configuration (ESPHome reference sequence)
    // ========================================================================
    ES7210_setGain(config->mics, config->gain);

    // ========================================================================
    // 11) Mic power on (ESPHome: REG47-4A=0x08, then REG06=0x04, then REG4B/4C)
    // ========================================================================
    es7210_write_reg(ES7210_MIC1_POWER_REG47, (mics & ES7210_MIC1) ? 0x08 : 0x00);
    es7210_write_reg(ES7210_MIC2_POWER_REG48, (mics & ES7210_MIC2) ? 0x08 : 0x00);
    es7210_write_reg(ES7210_MIC3_POWER_REG49, (mics & ES7210_MIC3) ? 0x08 : 0x00);
    es7210_write_reg(ES7210_MIC4_POWER_REG4A, (mics & ES7210_MIC4) ? 0x08 : 0x00);

    // Power down DLL (ESPHome: REG06=0x04 after mic power regs)
    es7210_write_reg(ES7210_POWER_DOWN_REG06, 0x04);

    // Power on MIC bias + ADC + PGA
    es7210_write_reg(ES7210_MIC12_POWER_REG4B, 0x0F);
    es7210_write_reg(ES7210_MIC34_POWER_REG4C, 0x0F);

    // Un-gate all clocks before enabling device
    es7210_write_reg(ES7210_CLOCK_OFF_REG01, 0x00);
    delay(10);

    // ========================================================================
    // 12) Enable device (ESPHome: REG00=0x71 then 0x41)
    // ========================================================================
    es7210_write_reg(ES7210_RESET_REG00, 0x71);
    delay(10);
    es7210_write_reg(ES7210_RESET_REG00, 0x41);
    delay(10);

    Serial.printf("ES7210: Mic power set — REG4B=0x%02X REG4C=0x%02X REG40=0x%02X REG06=0x%02X\n",
                  es7210_read_reg(ES7210_MIC12_POWER_REG4B),
                  es7210_read_reg(ES7210_MIC34_POWER_REG4C),
                  es7210_read_reg(ES7210_ANALOG_REG40),
                  es7210_read_reg(ES7210_POWER_DOWN_REG06));

    es7210_initialized = true;

    // Dump registers to verify init
    Serial.println("ES7210: Initialization complete — register dump:");
    ES7210_dumpRegisters();

    return true;
}

void ES7210_deinit(void) {
    if (!es7210_initialized) return;

    // Put device in reset/power down
    es7210_write_reg(ES7210_RESET_REG00, 0x00);  // Reset
    es7210_write_reg(ES7210_CLOCK_OFF_REG01, 0xFF);  // All clocks off
    es7210_write_reg(ES7210_ANALOG_REG40, 0x00);  // Analog power off

    es7210_initialized = false;
    Serial.println("ES7210: Deinitialized");
}

bool ES7210_start(void) {
    if (!es7210_initialized) return false;

    // Unmute ADCs
    es7210_write_reg(ES7210_ADC_AUTOMUTE_REG13, 0x00);
    es7210_write_reg(ES7210_ADC12_MUTERANGE_REG15, 0x00);
    es7210_write_reg(ES7210_ADC34_MUTERANGE_REG14, 0x00);

    // Enable clock
    es7210_write_reg(ES7210_CLOCK_OFF_REG01, 0x00);

    // Enable operation
    es7210_write_reg(ES7210_RESET_REG00, 0x41);

    Serial.println("ES7210: Started (unmuted)");
    return true;
}

bool ES7210_stop(void) {
    if (!es7210_initialized) return false;

    // Mute all ADCs
    es7210_write_reg(ES7210_ADC_AUTOMUTE_REG13, 0x0F);
    es7210_write_reg(ES7210_ADC12_MUTERANGE_REG15, 0xFF);
    es7210_write_reg(ES7210_ADC34_MUTERANGE_REG14, 0xFF);

    Serial.println("ES7210: Stopped (muted)");
    return true;
}

bool ES7210_setGain(es7210_mic_select_t mics, es7210_gain_t gain) {
    if (gain > ES7210_GAIN_37_5DB) {
        gain = ES7210_GAIN_37_5DB;
    }

    Serial.printf("ES7210: Setting gain to %ddB for mics 0x%02X\n", gain * 3, mics);

    if (mics & ES7210_MIC1) {
        es7210_write_reg(ES7210_MIC1_GAIN_REG43, gain | 0x10);
    }
    if (mics & ES7210_MIC2) {
        es7210_write_reg(ES7210_MIC2_GAIN_REG44, gain | 0x10);
    }
    if (mics & ES7210_MIC3) {
        es7210_write_reg(ES7210_MIC3_GAIN_REG45, gain | 0x10);
    }
    if (mics & ES7210_MIC4) {
        es7210_write_reg(ES7210_MIC4_GAIN_REG46, gain | 0x10);
    }

    return true;
}

bool ES7210_setMute(bool mute) {
    if (!es7210_initialized) return false;

    if (mute) {
        es7210_write_reg(ES7210_ADC12_MUTERANGE_REG15, 0xFF);
        es7210_write_reg(ES7210_ADC34_MUTERANGE_REG14, 0xFF);
    } else {
        es7210_write_reg(ES7210_ADC12_MUTERANGE_REG15, 0x00);
        es7210_write_reg(ES7210_ADC34_MUTERANGE_REG14, 0x00);
    }

    return true;
}

uint8_t ES7210_readReg(uint8_t reg) {
    return es7210_read_reg(reg);
}

bool ES7210_writeReg(uint8_t reg, uint8_t value) {
    return es7210_write_reg(reg, value);
}

void ES7210_dumpRegisters(void) {
    Serial.println("ES7210: Register Dump");
    Serial.println("====================");

    const uint8_t regs[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C
    };

    for (size_t i = 0; i < sizeof(regs); i++) {
        uint8_t val = es7210_read_reg(regs[i]);
        Serial.printf("  REG[0x%02X] = 0x%02X\n", regs[i], val);
    }
}

#endif // PLATFORM_ESP32
