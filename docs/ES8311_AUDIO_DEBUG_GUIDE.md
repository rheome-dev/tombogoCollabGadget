# ES8311 Audio Codec Debug Guide

## Waveshare ESP32-S3-Touch-AMOLED-1.75

This documents the root causes and fixes for the ES8311 audio codec failing to initialize on the Waveshare ESP32-S3-Touch-AMOLED-1.75 board.

## Root Causes

### 1. I2C SDA/SCL Pins Swapped

**Symptom:** ES8311 (and potentially other devices) not found on I2C scan.

The Waveshare board uses:
- **SDA = GPIO 15**
- **SCL = GPIO 14**

Arduino's `Wire.begin(sda, scl)` takes SDA first, SCL second. The incorrect call was:

```cpp
// WRONG — pins are backwards
Wire.begin(14, 15, 400000);

// CORRECT
Wire.begin(15, 14, 400000);
```

**Why it partially worked:** The TCA9554 IO expander (used for display power) appeared to respond even with swapped pins because simple write-only I2C transactions can sometimes succeed when SDA/SCL are swapped on ESP32 (the GPIO matrix routes signals through the peripheral, and short transactions may happen to be interpreted correctly). The ES8311, which requires multi-byte read/write sequences with register addressing, fails reliably.

### 2. ES8311 I2C Address: 7-bit vs 8-bit

**Symptom:** I2C scan shows NACK at address 0x30.

The ES8311 datasheet specifies:
- **7-bit address:** `0x18` (CE pin low) or `0x19` (CE pin high)
- **8-bit write address:** `0x30` (which is `0x18 << 1`)

Arduino's Wire library uses **7-bit addresses**. The Waveshare schematic shows `0x30` which is the 8-bit format. The code incorrectly used this directly:

```cpp
// WRONG — 0x30 is the 8-bit address
#define ES8311_ADDR 0x30

// CORRECT — Arduino Wire uses 7-bit
#define ES8311_ADDR 0x18
```

The Waveshare official example confirms this: `ES8311_ADDRRES_0 = 0x18`.

## Working I2C Bus Map

After fixes, the internal I2C bus (SDA=15, SCL=14) shows:

| Address | Device | Notes |
|---------|--------|-------|
| 0x18 | ES8311 / TCA9554 | Audio DAC + IO expander share address |
| 0x34 | Unknown | Possibly ES8311 alternate register bank |
| 0x40 | ES7210 | 4-channel ADC (microphone array) |
| 0x51 | PCF85063 | RTC |
| 0x5A | CST9217 | Touch controller |
| 0x6B | QMI8658 | 6-axis IMU |

External I2C bus (SDA=16, SCL=17):

| Address | Device | Notes |
|---------|--------|-------|
| 0x27 | MCP23017 | 16-bit IO expander (buttons/joystick/encoder) |

## Audio Initialization Sequence

The working initialization order (matching Waveshare official example):

1. **Enable PA amplifier pin** (GPIO 46) — set LOW initially
2. **Start MCLK** via LEDC on GPIO 42 at `sample_rate * 256` Hz
3. **Wait 10ms** for clock to stabilize
4. **Initialize ES8311** via I2C at 0x18:
   - Reset codec
   - Configure clock (MCLK from pin, not SCLK)
   - Set audio format (16-bit I2S)
   - Power up analog stages
   - Set volume and mic gain
5. **Initialize ES7210** (optional, for microphone input)
6. **Configure I2S** peripheral:
   - BCLK = GPIO 9
   - WS (LRCK) = GPIO 45
   - DOUT (to codec) = GPIO 8
   - DIN (from codec) = GPIO 10
7. **To start playback:** unmute ES8311, set PA HIGH, write samples via I2S

## Pin Reference (Audio-Related)

| Function | GPIO | Direction |
|----------|------|-----------|
| I2S MCLK | 42 | Output (LEDC) |
| I2S BCLK | 9 | Output |
| I2S LRCK/WS | 45 | Output |
| I2S DOUT (to ES8311) | 8 | Output |
| I2S DIN (from ES7210) | 10 | Input |
| PA Enable (NS4150B) | 46 | Output (HIGH=on) |
| I2C SDA | 15 | Bidirectional |
| I2C SCL | 14 | Output |

## Common Pitfalls

1. **MCLK must be running before ES8311 init.** The codec needs its master clock to respond to I2C commands properly.

2. **Wire.begin() order matters.** If you call `Wire.begin()` for display init first, the second call logs a warning ("Bus already started") but doesn't change pins. Initialize Wire once with the correct pins before any I2C operations.

3. **The Waveshare official ES8311 driver uses ESP32 HAL I2C functions** (`i2cWrite`, `i2cWriteReadNonStop`) rather than Arduino Wire. The adapted version in this project uses Wire and works correctly with the 7-bit address.

4. **USB CDC serial.** Boot messages are lost if `Serial` isn't connected at startup. Use `while(!Serial && millis() < 5000) delay(10);` to wait for USB enumeration.

## Test Procedure

1. Flash `firmware/tests/mcp_test/`
2. Open serial monitor at 115200 baud
3. Verify boot log shows `[ES8311] Found at 0x18!` and `[ES8311] Init OK`
4. Press encoder switch (MCP pin 0) to start audio engine
5. Hold encoder B (MCP pin 1) to play 1kHz sine wave through speaker
6. If using headphone jack inline with speaker, sound should be present in both
