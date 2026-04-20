/**
 * WM8960 Minimal Audio Test - Phase 1
 *
 * Standalone test sketch for verifying WM8960 headphone output.
 * This file can be used standalone (as main.cpp replacement) or
 * integrated into the main firmware after Phase 1 verification.
 *
 * Test sequence:
 * 1. Initialize I2C on GPIO15(SDA)/GPIO14(SCL)
 * 2. Initialize I2S1 on GPIO16(WS)/GPIO17(BCLK)/GPIO18(DOUT)
 * 3. Initialize WM8960 via I2C
 * 4. Generate 1kHz sine wave and play through WM8960 headphone output
 *
 * Hardware connections (see docs/ESP32S3_WM8960_MCP23017_Connection_Guide.md):
 * - I2C: GPIO15(SDA), GPIO14(SCL) - shared with MCP23017 and onboard devices
 * - I2S1: GPIO16(WS), GPIO17(BCLK), GPIO18(DOUT) - H2 header
 * - WM8960 address: 0x1A
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include "platforms/esp32/wm8960.h"

// I2C pins (underneath pads - shared I2C bus)
#define I2C_SDA 15
#define I2C_SCL 14

// I2S1 pins (H2 header - I2S0 is used by onboard ES8311)
#define I2S_BCLK    17
#define I2S_LRCK    16
#define I2S_DOUT    18  // Data output to WM8960 TXSDA (DACDAT)
#define I2S_PORT    I2S_NUM_1

// Audio buffer settings
#define SAMPLE_RATE         44100
#define BUFFER_SIZE         512
#define TONE_FREQUENCY      1000    // 1kHz test tone

// I2S configuration for WM8960
static i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Stereo
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
};

static i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRCK,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE  // No input (playback only)
};

// Sine wave lookup table for 1kHz tone at 44.1kHz sample rate
// 44.1kHz / 1kHz = 44.1 samples per cycle
#define SINE_TABLE_SIZE 44
static int16_t sine_table[SINE_TABLE_SIZE * 2];  // Stereo interleaved

// Generate stereo sine wave table
void generate_sine_table(void) {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        float sample = sinf(2.0f * PI * i / SINE_TABLE_SIZE);
        int16_t s = (int16_t)(sample * 16000);  // ~-3dB to avoid clipping
        sine_table[i * 2] = s;         // Left channel
        sine_table[i * 2 + 1] = s;     // Right channel
    }
}

// Initialize I2S1 for WM8960 (separate from onboard ES8311 which uses I2S0)
void initI2S(void) {
    Serial.println("ESP32: Initializing I2S1 for WM8960...");

    // Install I2S driver
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("ESP32: I2S driver install failed: %d\n", err);
        return;
    }

    // Set I2S pins
    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("ESP32: I2S pin config failed: %d\n", err);
        return;
    }

    // Clear DMA buffer to avoid initial noise
    i2s_zero_dma_buffer(I2S_PORT);

    Serial.println("ESP32: I2S1 initialized (GPIO16=WS, GPIO17=BCLK, GPIO18=DOUT)");
}

// Play a block of audio data
void playAudioBlock(const int16_t* data, size_t samples) {
    size_t bytes_written = 0;
    i2s_write(I2S_PORT, data, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}

// Test tone generator state
static int sine_index = 0;

void playTestTone(size_t block_size) {
    // Generate and play a block of sine wave data
    int16_t buffer[SINE_TABLE_SIZE * 2];  // One complete cycle

    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        int idx = (sine_index + i) % SINE_TABLE_SIZE;
        buffer[i * 2] = sine_table[idx * 2];       // Left
        buffer[i * 2 + 1] = sine_table[idx * 2 + 1]; // Right
    }

    sine_index = (sine_index + SINE_TABLE_SIZE) % SINE_TABLE_SIZE;
    playAudioBlock(buffer, SINE_TABLE_SIZE * 2);
}

void setup() {
    Serial.begin(115200);
    delay(500);  // Give serial time to stabilize

    Serial.println("\n=== WM8960 Phase 1 Test ===");
    Serial.println("Goal: Audio signal out of WM8960 headphone jack\n");

    // 1. Initialize I2C on GPIO15(SDA)/GPIO14(SCL)
    Serial.println("Step 1: Initializing I2C...");
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);  // 400kHz fast mode
    Serial.printf("I2C: GPIO15(SDA)/GPIO14(SCL) at 400kHz\n");

    // 2. Initialize WM8960 via I2C
    Serial.println("\nStep 2: Initializing WM8960...");
    if (!WM8960_init()) {
        Serial.println("ERROR: WM8960 initialization failed!");
        while (1) {
            delay(1000);
            Serial.println("WM8960 not detected - check wiring (SDA/SCL to underneath pads)");
        }
    }
    Serial.println("WM8960: Ready");

    // 3. Initialize I2S1 for WM8960 audio
    Serial.println("\nStep 3: Initializing I2S1...");
    initI2S();
    Serial.printf("I2S1: %dHz, 16-bit stereo, I2S master\n", SAMPLE_RATE);

    // 4. Generate sine wave table
    Serial.println("\nStep 4: Generating 1kHz test tone...");
    generate_sine_table();
    Serial.println("Tone: 1kHz sine wave at ~-3dB");

    // 5. Play test tone
    Serial.println("\n=== Playing test tone ===");
    Serial.println("Listen for 1kHz tone in headphones...");
    Serial.println("If no audio: check headphone connection, try different headphones");
    Serial.println("If distorted: reduce volume or check sample rate\n");

    // Unmute WM8960
    WM8960_setMute(false);

    // Play several cycles of the sine wave
    for (int j = 0; j < 100; j++) {
        playTestTone(SINE_TABLE_SIZE);
    }

    Serial.println("Test tone played. Check headphone output.");
    Serial.println("\n=== Phase 1 Complete ===");
    Serial.println("If you hear audio: WM8960 and I2S1 are working.");
    Serial.println("If no audio: See troubleshooting in connection guide.");
}

void loop() {
    // Continuously play a short burst every 2 seconds to indicate system is alive
    static uint32_t lastPlay = 0;
    static bool ledState = false;

    if (millis() - lastPlay > 2000) {
        lastPlay = millis();

        // Play a short beep
        WM8960_setMute(false);
        for (int j = 0; j < 10; j++) {
            playTestTone(SINE_TABLE_SIZE);
        }
        WM8960_setMute(true);

        // Toggle built-in LED to show activity (if available)
        ledState = !ledState;
        // digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);  // Uncomment if using LED

        Serial.printf("[%lu] Heartbeat - WM8960 alive\n", millis() / 1000);
    }
}
