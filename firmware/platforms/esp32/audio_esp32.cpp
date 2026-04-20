/**
 * ESP32 Platform - Audio HAL Implementation
 *
 * Uses ESP32 I2S driver with dual codec setup:
 *   - ES8311 (DAC): Speaker output at I2C 0x30
 *   - ES7210 (ADC): 4-channel microphone array at I2C 0x40
 *
 * Includes MCLK generation via LEDC and PA_EN control
 * Reference: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
 */

#ifdef PLATFORM_ESP32

#include "../../hal/audio_hal.h"
#include "pin_config.h"
#include "../../src/config.h"
#include "es7210.h"
#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <Wire.h>

// Audio state
static bool audioInitialized = false;
static bool audioRunning = false;
static bool es7210Present = false;
static uint8_t currentVolume = 50;

// I2S configuration
static const i2s_port_t I2S_PORT = I2S_NUM_0;

// MCLK configuration (12.288 MHz for 16kHz sample rate, 256x oversampling)
// For 16kHz: MCLK = 16000 * 256 = 4.096 MHz (minimum)
// For 44.1kHz: MCLK = 44100 * 256 = 11.2896 MHz
// Using 12.288 MHz as it works for both (common audio clock)
static const uint32_t MCLK_FREQ = 12288000;  // 12.288 MHz
static const ledc_channel_t MCLK_LEDC_CHANNEL = LEDC_CHANNEL_0;
static const ledc_timer_t MCLK_LEDC_TIMER = LEDC_TIMER_0;

// ES8311 codec I2C address
static const uint8_t ES8311_I2C_ADDR = 0x30;

// ES8311 I2C register write
static bool es8311_write_reg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(ES8311_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

// ES8311 I2C register read
static uint8_t es8311_read_reg(uint8_t reg) {
    Wire.beginTransmission(ES8311_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)ES8311_I2C_ADDR, (uint8_t)1);
    return Wire.read();
}

// Initialize MCLK output using LEDC
static bool init_mclk(void) {
    Serial.println("ESP32: Initializing MCLK via LEDC...");

    // Configure LEDC timer for MCLK generation
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_1_BIT,  // 1-bit for 50% duty (square wave)
        .timer_num = MCLK_LEDC_TIMER,
        .freq_hz = MCLK_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        Serial.printf("ESP32: MCLK timer config failed: %d\n", err);
        return false;
    }

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num = MCLK_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = MCLK_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MCLK_LEDC_TIMER,
        .duty = 1,  // 50% duty for 1-bit resolution
        .hpoint = 0
    };

    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        Serial.printf("ESP32: MCLK channel config failed: %d\n", err);
        return false;
    }

    Serial.printf("ESP32: MCLK initialized at %d Hz on GPIO%d\n", MCLK_FREQ, MCLK_PIN);
    return true;
}

// Initialize PA (Power Amplifier) enable GPIO
static void init_pa_en(void) {
    gpio_set_direction((gpio_num_t)PA_EN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PA_EN, 0);  // Start with PA disabled
    Serial.printf("ESP32: PA_EN initialized on GPIO%d (disabled)\n", PA_EN);
}

// Enable/disable power amplifier
static void set_pa_enabled(bool enabled) {
    gpio_set_level((gpio_num_t)PA_EN, enabled ? 1 : 0);
    Serial.printf("ESP32: PA %s\n", enabled ? "enabled" : "disabled");
}

// Initialize ES8311 audio codec
static bool es8311_init_codec(void) {
    Serial.println("ESP32: Initializing ES8311 codec...");

    // Check ES8311 presence
    Wire.beginTransmission(ES8311_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("ESP32: ES8311 not found!");
        return false;
    }

    // Reset ES8311
    es8311_write_reg(0x00, 0x80);  // Soft reset
    delay(20);
    es8311_write_reg(0x00, 0x00);  // Clear reset
    delay(10);

    // ES8311 Register Configuration (based on datasheet)
    // Reference: ES8311 Datasheet Section 8

    // System Clock Configuration (Reg 0x01)
    // Use MCLK as clock source
    es8311_write_reg(0x01, 0x3F);  // Normal operation, internal clock enabled

    // Clock Manager 1 (Reg 0x02) - MCLK divider
    // MCLK = 12.288MHz, fs = 16kHz, ratio = 768
    es8311_write_reg(0x02, 0x00);  // MCLK/1

    // Clock Manager 2 (Reg 0x03) - BCLK/LRCK ratio
    // For 16-bit stereo: BCLK = fs * 32 = 512kHz
    es8311_write_reg(0x03, 0x10);  // ADC/DAC clock div

    // Clock Manager 3 (Reg 0x04) - NFS
    es8311_write_reg(0x04, 0x10);

    // Clock Manager 4 (Reg 0x05) - ADC clock
    es8311_write_reg(0x05, 0x00);

    // SDI/SDP (Reg 0x06) - Serial Data Interface
    es8311_write_reg(0x06, 0x03);  // Slave mode, I2S 16-bit

    // SDP Out (Reg 0x07) - I2S format
    es8311_write_reg(0x07, 0x00);  // I2S mode, 16-bit

    // ADC Control (Reg 0x09-0x0D)
    es8311_write_reg(0x09, 0x00);  // ADC power control
    es8311_write_reg(0x0A, 0x00);  // ADC input select
    es8311_write_reg(0x0B, 0x00);  // ADC volume (0dB)
    es8311_write_reg(0x0C, 0x00);  // ADC ALC settings
    es8311_write_reg(0x0D, 0x00);  // ADC HPF

    // DAC Control (Reg 0x10-0x14)
    es8311_write_reg(0x10, 0x1C);  // DAC soft ramp, OSR
    es8311_write_reg(0x11, 0x00);  // DAC output select

    // Set volume (0-100 mapped to 0xBF-0x00)
    uint8_t vol = (100 - currentVolume) * 191 / 100;
    es8311_write_reg(0x12, vol);   // DAC volume L
    es8311_write_reg(0x13, vol);   // DAC volume R

    // System Control (Reg 0x14-0x15)
    es8311_write_reg(0x14, 0x0A);  // Reference and bias

    // Analog Control (Reg 0x16-0x17)
    es8311_write_reg(0x16, 0x21);  // ADC analog
    es8311_write_reg(0x17, 0x01);  // DAC analog

    // Power On Sequence
    delay(50);
    es8311_write_reg(0x00, 0x80);  // Enable chip
    delay(10);

    Serial.println("ESP32: ES8311 codec initialized");
    return true;
}

void AudioHAL_init(void) {
    Serial.println("ESP32: Initializing Audio HAL...");

    // Initialize PA_EN GPIO (disabled initially)
    init_pa_en();

    // Initialize MCLK output BEFORE codec init (codec needs clock)
    if (!init_mclk()) {
        Serial.println("ESP32: MCLK init failed - audio may not work correctly");
        // Continue anyway - some configurations may work without MCLK
    }

    // Small delay for MCLK to stabilize
    delay(10);

    // Note: I2C is already initialized in HAL_init()
    // Initialize ES8311 codec (DAC for speaker output)
    if (!es8311_init_codec()) {
        Serial.println("ESP32: Audio init failed - no ES8311 codec found");
        return;
    }

    // Initialize ES7210 codec (ADC for microphone array)
    es7210_config_t es7210_cfg = ES7210_CONFIG_DEFAULT();
    // MIC1 and MIC2 already set by default config
    es7210_cfg.gain = ES7210_GAIN_24DB;      // 24dB gain for MEMS mics
    es7210_cfg.sample_rate = (es7210_sample_rate_t)SAMPLE_RATE;
    es7210_cfg.bits = ES7210_BITS_16;
    es7210_cfg.i2s_format = ES7210_I2S_NORMAL;
    es7210_cfg.enable_tdm = false;           // Standard stereo mode (MIC1/MIC2)
    es7210_cfg.mclk_freq = MCLK_FREQ;

    if (ES7210_init(&es7210_cfg)) {
        es7210Present = true;
        Serial.println("ESP32: ES7210 microphone array initialized");
    } else {
        Serial.println("ESP32: ES7210 not found - microphone input may not work");
        // Continue anyway - ES8311 can still provide basic mic via its ADC
    }

    // Initialize I2S for audio
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = true,         // Use APLL for better audio quality
        .tx_desc_auto_clear = true,
        .fixed_mclk = MCLK_FREQ   // Match LEDC-generated MCLK
    };

    // Install I2S driver
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("ESP32: I2S driver install failed: %d\n", err);
        return;
    }

    // Configure I2S pins
    i2s_pin_config_t pinConfig = {
        .bck_io_num = BCLK_PIN,
        .ws_io_num = WS_PIN,
        .data_out_num = DOUT_PIN,
        .data_in_num = DIN_PIN
    };

    err = i2s_set_pin(I2S_PORT, &pinConfig);
    if (err != ESP_OK) {
        Serial.printf("ESP32: I2S pin config failed: %d\n", err);
        i2s_driver_uninstall(I2S_PORT);
        return;
    }

    // Zero-fill DMA buffers to prevent noise on startup
    i2s_zero_dma_buffer(I2S_PORT);

    audioInitialized = true;
    Serial.println("ESP32: Audio initialized successfully");
    Serial.printf("  Sample rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("  MCLK: %d Hz on GPIO%d\n", MCLK_FREQ, MCLK_PIN);
    Serial.printf("  I2S pins - BCLK: %d, WS: %d, DOUT: %d, DIN: %d\n",
                  BCLK_PIN, WS_PIN, DOUT_PIN, DIN_PIN);
    Serial.printf("  PA_EN: GPIO%d (disabled)\n", PA_EN);
    Serial.printf("  ES8311 DAC: 0x%02X (speaker)\n", ES8311_I2C_ADDR);
    Serial.printf("  ES7210 ADC: 0x%02X (%s)\n", ES7210_I2C_ADDR,
                  es7210Present ? "mic array active" : "not found");
}

void AudioHAL_start(void) {
    if (!audioInitialized) {
        return;
    }

    // Enable ES8311 recording/playback
    es8311_write_reg(0x00, 0x80);  // Enable chip
    es8311_write_reg(0x01, 0x3F);  // Enable all power

    // Enable ES7210 microphone array if present
    if (es7210Present) {
        ES7210_start();
    }

    // Small delay before enabling PA to prevent pop
    delay(50);

    // Enable power amplifier for speaker output
    set_pa_enabled(true);

    audioRunning = true;
    Serial.println("ESP32: Audio started (PA enabled, ES7210 active)");
}

void AudioHAL_stop(void) {
    if (!audioInitialized) {
        return;
    }

    // Disable power amplifier FIRST to prevent pop
    set_pa_enabled(false);
    delay(10);

    // Stop ES7210 microphone array if present
    if (es7210Present) {
        ES7210_stop();
    }

    // Then disable ES8311
    es8311_write_reg(0x01, 0x00);  // Power down
    es8311_write_reg(0x00, 0x00);  // Disable chip

    audioRunning = false;
    Serial.println("ESP32: Audio stopped (PA disabled, ES7210 muted)");
}

void AudioHAL_setVolume(uint8_t volume) {
    if (!audioInitialized) {
        return;
    }

    // Clamp volume to 0-100
    if (volume > 100) volume = 100;
    currentVolume = volume;

    // Convert 0-100 to ES8311 range (0xE0-0x00)
    uint8_t vol = 0xE0 - (volume * 224 / 100);
    es8311_write_reg(0x14, vol);
}

uint32_t AudioHAL_readMic(int16_t* buffer, uint32_t samples) {
    if (!audioInitialized || !audioRunning || !buffer) {
        return 0;
    }

    // Read from I2S
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(I2S_PORT, buffer, samples * sizeof(int16_t) * 2, &bytesRead, 10 / portTICK_PERIOD_MS);

    if (err != ESP_OK) {
        Serial.printf("ESP32: I2S read error: %d\n", err);
        return 0;
    }

    return bytesRead / (sizeof(int16_t) * 2);
}

void AudioHAL_writeSpeaker(const int16_t* buffer, uint32_t samples) {
    if (!audioInitialized || !audioRunning || !buffer) {
        return;
    }

    // Write to I2S
    size_t bytesWritten = 0;
    esp_err_t err = i2s_write(I2S_PORT, buffer, samples * sizeof(int16_t) * 2, &bytesWritten, 10 / portTICK_PERIOD_MS);

    if (err != ESP_OK) {
        Serial.printf("ESP32: I2S write error: %d\n", err);
    }
}

bool AudioHAL_micAvailable(void) {
    // ES7210 provides dedicated 4-channel mic input
    // Fallback to ES8311 ADC if ES7210 not present
    return audioInitialized && (es7210Present || true);
}

bool AudioHAL_speakerReady(void) {
    return audioInitialized;
}

#endif // PLATFORM_ESP32
