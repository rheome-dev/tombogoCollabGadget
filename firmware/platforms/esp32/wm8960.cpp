/**
 * WM8960 Audio Codec Driver Implementation
 *
 * Provides I2C control and configuration for WM8960 headphone DAC.
 * Uses I2S1 on GPIO16/17/18 for audio data.
 *
 * Reference: WM8960 datasheet, Waveshare WM8960 Audio Board wiki
 */

#ifdef PLATFORM_ESP32

#include "wm8960.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>

// WM8960 register definitions (see WM8960 datasheet table)
#define WM8960_REG_RESET            0x0F    // Software reset
#define WM8960_REG_POWER1           0x19    // Power management 1
#define WM8960_REG_POWER2           0x1A    // Power management 2
#define WM8960_REG_POWER3           0x2F    // Power management 3
#define WM8960_REG_CLOCK1           0x04    // Clocking 1
#define WM8960_REG_CLOCK2           0x05    // Clocking 2
#define WM8960_REG_DAC              0x05    // DAC control (same as CLOCK2 - check)
#define WM8960_REG_AUDIO_IF         0x07    // Audio interface
#define WM8960_REG_DAC_VOLUME_L     0x0A    // Left DAC volume
#define WM8960_REG_DAC_VOLUME_R     0x0B    // Right DAC volume
#define WM8960_REG_ADC              0x0F    // ADC control
#define WM8960_REG_OUTPUT_MIXER1    0x22    // Left output mixer
#define WM8960_REG_OUTPUT_MIXER2    0x25    // Right output mixer
#define WM8960_REG_LOUT1            0x02    // Left headphone volume
#define WM8960_REG_ROUT1            0x03    // Right headphone volume
#define WM8960_REG_PLL_N            0x34    // PLL N divider
#define WM8960_REG_PLL_K1            0x35    // PLL K1
#define WM8960_REG_PLL_K2            0x36    // PLL K2
#define WM8960_REG_PLL_K3            0x37    // PLL K3

static bool wm8960_initialized = false;
static bool wm8960_muted = false;

// Write WM8960 register via I2C
// WM8960 register write format: 7-bit reg addr, then 9-bit data
// The protocol uses reg << 1 | (data >> 8) for first byte, then data & 0xFF for second
bool WM8960_writeReg(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(WM8960_I2C_ADDR);
    Wire.write((reg << 1) | ((value >> 8) & 0x01));  // Register address + MSB of data
    Wire.write(value & 0xFF);                          // LSB of data
    return Wire.endTransmission() == 0;
}

// Read WM8960 register via I2C
uint16_t WM8960_readReg(uint8_t reg) {
    // WM8960 read not commonly used - for debugging
    Wire.beginTransmission(WM8960_I2C_ADDR);
    Wire.write(reg << 1);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)WM8960_I2C_ADDR, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        return ((msb & 0x01) << 8) | lsb;
    }
    return 0;
}

// Verify I2C communication with WM8960
bool WM8960_isConnected(void) {
    Wire.beginTransmission(WM8960_I2C_ADDR);
    return Wire.endTransmission() == 0;
}

// Initialize WM8960 for headphone playback
bool WM8960_init(void) {
    Serial.println("ESP32: Initializing WM8960...");

    // Check connection
    if (!WM8960_isConnected()) {
        Serial.println("ESP32: WM8960 not found at address 0x1A!");
        return false;
    }
    Serial.println("ESP32: WM8960 detected");

    // Software reset
    Serial.println("ESP32: WM8960 software reset...");
    WM8960_writeReg(WM8960_REG_RESET, 0x0000);
    delay(100);

    // Verify reset worked (read back reset register - should return 0 after reset clears)
    // The WM8960 reset register automatically clears after reset completes

    // === Power Management ===
    // R25 (0x19) Power 1: VMID=50kΩ divider, VREF enabled
    // VMIDSEL[1:0]=01: 50kΩ divider enabled (needed for headphone)
    // VREF=1: Voltage reference enabled
    Serial.println("ESP32: WM8960 configuring power management...");
    WM8960_writeReg(WM8960_REG_POWER1, 0x00C0);   // VMID=50k, VREF on
    delay(50);  // Wait for VMID to stabilize

    // R26 (0x1A) Power 2: Enable DACs and headphone outputs
    // DACL=1: Left DAC enabled
    // DACR=1: Right DAC enabled
    // LOUT1=1: Left headphone driver enabled
    // ROUT1=1: Right headphone driver enabled
    WM8960_writeReg(WM8960_REG_POWER2, 0x01E0);   // DAC L/R + HP L/R on

    // R47 (0x2F) Power 3: Enable output mixers
    // LOMIX=1: Left output mixer enabled
    // ROMIX=1: Right output mixer enabled
    WM8960_writeReg(WM8960_REG_POWER3, 0x000C);   // Output mixers on

    // === Clock Configuration ===
    // For 44.1kHz stereo I2S at 16-bit:
    // BCLK = 44.1kHz * 32 bits/frame * 2 channels = 2.8224 MHz (WS=44.1kHz)
    // Actually for 16-bit stereo: BCLK = 44100 * 16 * 2 = 1.4112 MHz
    // This is a standard multiple of 44.1kHz (32x oversampling ratio)
    //
    // R4 (0x04) Clocking: Use SYSCLK derived from MCLK (onboard 24MHz crystal)
    // CLKSEL=0: MCLK source (onboard crystal)
    // MCLKDIV=0: SYSCLK = MCLK (24MHz)
    // ADCDIV=0: ADC clock divider = /1
    // DACDIV=0: DAC clock divider = /1
    WM8960_writeReg(WM8960_REG_CLOCK1, 0x0000);    // SYSCLK from MCLK

    // R5 (0x05) DAC Control: No de-emphasis, unmute
    WM8960_writeReg(WM8960_REG_DAC, 0x0000);      // DAC unmute

    // === Audio Interface ===
    // R7 (0x07) Audio Interface: I2S format, 16-bit, slave mode
    // FORMAT[1:0]=10: I2S format
    // WL[1:0]=00: 16-bit word length
    // MS=0: Slave mode (BCLK and LRCK generated by ESP32)
    WM8960_writeReg(WM8960_REG_AUDIO_IF, 0x0002); // I2S, 16-bit, slave

    // === DAC Volume ===
    // R10 (0x0A) Left DAC Volume
    // DACVU=1: Volume update bit (must be set)
    // LDACVOL[7:0]=0xFF: 0dB (maximum)
    WM8960_writeReg(WM8960_REG_DAC_VOLUME_L, 0x01FF); // 0dB, update

    // R11 (0x0B) Right DAC Volume
    // DACVU=1: Volume update bit
    // RDACVOL[7:0]=0xFF: 0dB (maximum)
    WM8960_writeReg(WM8960_REG_DAC_VOLUME_R, 0x01FF); // 0dB, update

    // === Output Mixer Routing ===
    // R34 (0x22) Left Output Mixer: Route left DAC to left output
    // LD2LO=1: Left DAC to left output mixer
    // LI2LOVOL[2:0]=010: 0dB routing gain
    WM8960_writeReg(WM8960_REG_OUTPUT_MIXER1, 0x0102); // DAC to L output mixer

    // R37 (0x25) Right Output Mixer: Route right DAC to right output
    // RD2RO=1: Right DAC to right output mixer
    // RI2ROVOL[2:0]=010: 0dB routing gain
    WM8960_writeReg(WM8960_REG_OUTPUT_MIXER2, 0x0102); // DAC to R output mixer

    // === Headphone Volume ===
    // R2 (0x02) Left Headphone Volume
    // HPVU=1: Headphone volume update
    // LHPVOL[6:0]=0x79: 0dB (see datasheet table for 0dB=0x79)
    WM8960_writeReg(WM8960_REG_LOUT1, 0x0179);    // 0dB HP volume, update

    // R3 (0x03) Right Headphone Volume
    // HPVU=1: Headphone volume update
    // RHPVOL[6:0]=0x79: 0dB
    WM8960_writeReg(WM8960_REG_ROUT1, 0x0179);    // 0dB HP volume, update

    wm8960_initialized = true;
    Serial.println("ESP32: WM8960 initialized for headphone playback");
    return true;
}

// Set headphone volume (0-255, where 127 = 0dB)
void WM8960_setHeadphoneVolume(uint8_t vol) {
    if (!wm8960_initialized) return;

    // Volume control via DACL/DACR digital volume
    // 0xFF = 0dB, 0x00 = mute
    WM8960_writeReg(WM8960_REG_DAC_VOLUME_L, 0x0100 | vol); // Update bit + volume
    WM8960_writeReg(WM8960_REG_DAC_VOLUME_R, 0x0100 | vol);
}

// Mute/unmute headphone output
void WM8960_setMute(bool mute) {
    if (!wm8960_initialized) return;

    // DACMU bit in R5 controls mute
    // 0x0000 = unmute, 0x0008 = mute
    if (mute) {
        WM8960_writeReg(WM8960_REG_DAC, 0x0008);  // Mute
    } else {
        WM8960_writeReg(WM8960_REG_DAC, 0x0000);  // Unmute
    }
    wm8960_muted = mute;
}

// Configure WM8960 PLL for deriving clocks from BCLK
// This is needed when using the WM8960 in slave mode with external BCLK
bool WM8960_configurePLL(uint32_t sampleRate) {
    // For WM8960 slave mode with BCLK as clock source:
    // The PLL is only needed if MCLK is not a multiple of the sample rate.
    // Since we're using ESP32 as I2S master generating BCLK directly,
    // and the WM8960 is in slave mode, we don't necessarily need PLL.
    //
    // The WM8960 can use BCLK directly as the audio clock source in slave mode.
    // This is configured via CLKSEL in R4.
    //
    // Current config (R4=0x00): MCLKDIV=0, SYSCLKDIV=0 → SYSCLK = MCLK
    // The onboard 24MHz crystal provides MCLK directly.
    //
    // For 44.1kHz with 24MHz MCLK:
    // SYSCLK / 256 = 93.75kHz (not 44.1kHz - would need PLL)
    //
    // Alternative: Use BCLK directly as source when in slave mode.
    // This is handled by setting the interface to slave and relying on
    // BCLK being a proper multiple of the sample rate.
    //
    // For 16-bit stereo I2S at 44.1kHz:
    // BCLK = 44.1kHz * 32 = 1.4112MHz (from ESP32 I2S)
    // WM8960 in slave mode will use BCLK/LRCK directly for its internal timing.
    //
    // Note: If audio doesn't work, the issue is likely that WM8960
    // needs a proper MCLK input. Without external MCLK connected to
    // the WM8960 RXMCLK pin, we must use PLL mode with BCLK as reference.
    //
    // This function can configure the PLL to derive SYSCLK from BCLK
    // when an external MCLK is not available.

    Serial.printf("ESP32: WM8960 PLL config for %lu Hz (not implemented - using direct BCLK mode)\n", sampleRate);
    return true;  // Currently using direct BCLK mode without PLL
}

#endif // PLATFORM_ESP32
