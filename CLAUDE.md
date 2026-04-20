# CLAUDE.md

## MANDATORY: Use td for Task Management

You must run td usage --new-session at conversation start (or after /clear) to see current work.
Use td usage -q for subsequent reads.

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Tombogo x Rheome Collab Gadget** is a wearable audio creativity device - a retro-active looper with resonator, rhythmic chopping, sequencable effects, and drum machine in a Tamagotchi-inspired lo-fi aesthetic.

- **Hardware**: Waveshare ESP32-S3-Touch-AMOLED-1.75 (ESP32-S3R8 dual-core 240MHz, 8MB PSRAM, 16MB Flash)
- **Display**: 1.75" AMOLED 466×466 capacitive touchscreen (CO5300 driver, QSPI)
- **Audio (Onboard)**:
  - **ES8311** (DAC) - I2C 0x30 → speaker output
  - **ES7210** (4-ch ADC) - I2C 0x40 → microphone array with AEC
  - **NS4150B** (PA) - GPIO46 → 3W speaker amplifier
- **Audio (External)**: WM8960 Audio Board (I2C 0x1A) → headphones
- **IMU**: QMI8658 6-axis accelerometer/gyroscope (I2C 0x6B)
- **IO Expander**: TCA9554 (I2C 0x18) for LCD power/reset
- **Framework**: Arduino with FreeRTOS (PlatformIO)

## Commands

### Build & Upload (PlatformIO)
```bash
# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean
```

### DSP Development (Faust)
```bash
# Compile Faust DSP to C++
faust -cn resonator -i -a cpp resonator.dsp > resonator.cpp

# Compile with higher optimization for ESP32
faust -cn name -i -a cpp -arch -double name.dsp > name.cpp
```

## Architecture

### Hardware Abstraction Layer (HAL)
All hardware access goes through HAL interfaces in `firmware/hal/` with ESP32 implementations in `firmware/platforms/esp32/`:

| Interface | Purpose |
|-----------|---------|
| `audio_hal.h` | I2S microphone/speaker interface |
| `display_hal.h` | LVGL framebuffer & pixel drawing |
| `imu_hal.h` | Accelerometer/gyroscope reading |
| `touch_hal.h` | Capacitive touch input |
| `storage_hal.h` | SD card file operations |

**Pattern**: Abstract interface in `firmware/hal/*.h`, implementations in `firmware/platforms/esp32/*.cpp`

### Core Systems
- **AudioEngine** (`firmware/src/core/audio_engine.cpp`): Central hub for DSP processing, mixing, parameter routing
- **RetroactiveBuffer** (`firmware/src/core/retroactive_buffer.cpp`): Circular buffer capturing last N seconds of audio (~480KB @ 16kHz)
- **UIManager** (`firmware/src/core/ui_manager.cpp`): Touch/IMU input handling, LVGL display updates

### FreeRTOS Task Architecture
- **Core 0**: UI loop (touch, IMU, display), IMU task (10ms loop)
- **Core 1**: Audio task (1ms loop, high priority)

### DSP Architecture (Faust)
Five independent DSP engines (`.dsp` files → compiled to C++):
- **Resonator**: Scale-locked, multi-mode
- **Chopper**: Rhythmic slicing (XLN Life-style)
- **Synth**: Oscillators + ADSR
- **Drums**: 16-step sequencer
- **Effects Chain**: Filter, delay, reverb, distortion, bitcrusher, granular

**Cross-platform**: Same Faust code compiles to ESP32 C++, native C++ (custom PCB), WebAssembly (companion web app)

## Key Files

| File | Purpose |
|------|---------|
| `firmware/src/main.cpp` | Entry point, FreeRTOS task setup |
| `firmware/src/config.h` | Compile-time feature flags & constants |
| `firmware/platforms/esp32/pin_config.h` | GPIO pin mappings |
| `firmware/platforms/esp32/audio_esp32.cpp` | ES8311 codec driver & I2S setup |
| `docs/PRD.md` | Full product requirements |
| `docs/AUDIO_ARCHITECTURE.md` | Dual codec design & implementation plan |
| `docs/FAUST_INTEGRATION.md` | DSP development guide |

## Important Patterns

### HAL Function Naming
`ComponentHAL_functionName()` (e.g., `AudioHAL_readMic()`)

### IMU Data Structure
```cpp
struct IMUData {
    float ax, ay, az;    // Accelerometer (g)
    float gx, gy, gz;   // Gyroscope (deg/s)
    float pitch, roll;  // Computed angles
};
```

### Configuration
Edit `firmware/src/config.h` for:
- `PLATFORM_ESP32` / `PLATFORM_CUSTOM_PCB`
- `SAMPLE_RATE` (16000 for ESP32, 44100 for custom PCB)
- `RETROACTIVE_BUFFER_SEC`
- Screen dimensions

### GPIO Pin Reference (Verified from Schematic 2026-02-17)

| Function | GPIO | Notes |
|----------|------|-------|
| **I2S Audio** |
| I2S_DSDIN | 8 | Data to ES8311 (speaker) |
| I2S_SCLK | 9 | Bit clock |
| I2S_ASDOUT | 10 | Data from ES7210 (mic) |
| I2S_MCLK | 42 | 12.288 MHz via LEDC |
| I2S_LRCK | 45 | Word select |
| PA_CTRL | 46 | NS4150B enable |
| **I2C Bus** |
| SCL | 14 | Shared by all I2C devices |
| SDA | 15 | Shared by all I2C devices |
| **Display (QSPI)** |
| QSPI_SIO0-3 | 4,5,6,7 | Data lines |
| QSPI_SCL | 38 | Clock |
| LCD_CS | 12 | Chip select |
| LCD_TE | 13 | Tearing effect |
| LCD_RESET | 39 | Hardware reset |
| **Touch** |
| TP_INT | 11 | Interrupt |
| TP_RESET | 40 | Reset |
| **SD Card (SPI)** |
| MOSI | 1 | |
| SCK | 2 | |
| MISO | 3 | |
| CS | 41 | |
| **IMU** |
| QMI_INT2 | 21 | Direct GPIO interrupt |

### GPIO Constraints
Nearly all 26 GPIO are allocated. For physical buttons, use PCF8574 I2C GPIO expander at 0x20 on existing I2C bus.

## Feature Status

**HAL Layer (Implemented)**:
- ES8311 codec driver (speaker output, mic input)
- MCLK generation via LEDC (12.288 MHz)
- PA_EN power amplifier control
- TCA9554 IO expander (LCD power/reset)
- LVGL display driver integration
- Touch input (CST9217)
- IMU input (QMI8658)
- SD card storage (SPI mode)
- PCF8574 button driver

**Pending HAL**:
- ES7210 ADC driver (4-channel microphone input)
- WM8960 codec driver (headphone output)

**Core Features (Pending)**:
- Audio playback/recording test
- Scale-locked resonator
- Rhythmic chop engine
- Simple synth
- Drum sample engine
- Effects chain
- WiFi/Bluetooth sync

## References

- XLN Audio Life - Rhythmic chopping paradigm
- Tim Exile Endless - Retroactive looping paradigm
- Devious Machines Infiltrator - Sequencable effects
