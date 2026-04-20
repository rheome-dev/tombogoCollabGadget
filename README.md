# Tombogo x Rheome Collab Gadget

A wearable audio creativity device - a retro-active looper with resonator, rhythmic chopping, sequencable effects, and drum machine in a Tamagotchi-inspired lo-fi aesthetic.

## Quick Start

### Prerequisites

- Arduino IDE with ESP32 board support
- Waveshare ESP32-S3-Touch-AMOLED-1.75
- Faust (for DSP development)

### Build

1. Open `firmware/platforms/esp32/esp32_main.ino` in Arduino IDE
2. Install required libraries:
   - LVGL
   - Arduino_GFX
   - ESP_I2S
   - SensorQMI8658
   - TouchDrvCSTXXX
3. Build and upload

## Project Structure

```
tombogoCollabGadget/
├── docs/
│   ├── PRD.md              # Product Requirements Document
│   └── FAUST_INTEGRATION.md # DSP development guide
├── firmware/
│   ├── src/
│   │   ├── main.cpp        # Entry point
│   │   ├── config.h        # System configuration
│   │   ├── core/           # Core systems
│   │   │   ├── audio_engine.cpp
│   │   │   ├── retroactive_buffer.cpp
│   │   │   └── ui_manager.cpp
│   │   └── audio/
│   │       └── dsp/        # DSP modules
│   ├── hal/                # Hardware abstraction interfaces
│   └── platforms/
│       ├── esp32/          # ESP32 implementation
│       └── custom_pcb/     # Future custom PCB
└── README.md
```

## Features

### Priority 1 - Core
- [x] Touch screen melody drawing
- [x] IMU motion control
- [ ] Retroactive record buffer

### Priority 2 - Resonator
- [ ] Scale-locked resonator from mic input
- [ ] Multiple resonator modes
- [ ] Scale selection

### Priority 3 - Audio Engines
- [ ] Rhythmic chop engine (XLN Life-style)
- [ ] Simple synth engine
- [ ] Drum sample engine
- [ ] Sequencable effects chain

### Priority 4 - Controls
- [ ] 4 physical buttons
- [ ] Advanced gesture control

### Priority 5 - Storage & Sync
- [ ] SD card save/load
- [ ] WiFi/Bluetooth cloud sync

### Priority 6 - UI/UX
- [ ] Dead-front display aesthetic
- [ ] Waveform visualization
- [ ] Effect parameter displays

## Hardware

### Current Prototype
- **Waveshare ESP32-S3-Touch-AMOLED-1.75**
- 1.75" AMOLED 466×466 capacitive touch
- Dual microphone array with echo cancellation
- 6-axis IMU
- 8MB PSRAM, 16MB Flash

### Target Custom PCB
- Different display
- Different audio codec
- Custom I/O configuration
- Optimized for manufacturing

## Documentation

- [Product Requirements Document](docs/PRD.md)
- [Faust DSP Integration](docs/FAUST_INTEGRATION.md)

## References

- XLN Audio Life - Rhythmic chopping
- Tim Exile Endless - Retroactive looping
- Devious Machines Infiltrator - Sequencable effects
- Terra AI Compass - Dead-front display
- Teenage Engineering TP-7 - Aesthetic reference

## License

Proprietary - Tombogo x Rheome Collaboration
