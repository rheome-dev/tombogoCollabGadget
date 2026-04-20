# Tombogo x Rheome Collab Gadget - Product Requirements Document

**Version:** 1.0
**Date:** 2026-02-16
**Status:** Draft
**Target Launch:** November 2025
**Target Cost:** $50-60 (75% margin at $250 price point)

---

## 1. Executive Summary

A wearable audio creativity device that enables users to capture, loop, and manipulate environmental sounds through an intuitive touch and motion interface. The device combines retro-active recording with real-time DSP processing (resonator, rhythmic chopping, synth engines, and sequencable effects) in a Tamagotchi-inspired lo-fi aesthetic.

---

## 2. Hardware Specifications

### Current Prototype (Waveshare ESP32-S3 AMOLED)
- **Processor:** ESP32-S3R8 dual-core 240MHz
- **Memory:** 8MB PSRAM, 16MB Flash
- **Display:** 1.75" AMOLED 466×466 capacitive touch (CO5300 driver)
- **Audio (Onboard):**
  - **ES8311** (DAC/Codec) - Speaker output, I2C address: 0x30
  - **ES7210** (4-ch ADC) - Microphone array input with AEC support, I2C address: 0x40
  - **NS4150B** (PA) - 3W Class D amplifier for speaker, controlled via GPIO46
- **Audio (External):** Waveshare WM8960 Audio Board
  - Stereo headphone output (40mW @ 16Ω)
  - Optional speaker output (1W @ 8Ω BTL)
  - I2C address: 0x1A
- **Sensors:** QMI8658 6-axis IMU (accelerometer + gyroscope)
- **IO Expander:** TCA9554 (0x18) for LCD power/reset control
- **Connectivity:** WiFi, Bluetooth 5, USB-C
- **Storage:** TF card slot (SD_MMC 1-bit mode)

### Target Custom PCB (Future)
- Different display (TBD based on cost/availability)
- Different audio codec (TBD)
- Custom I/O configuration
- Optimized for manufacturing

---

## 3. Feature Requirements

### Priority 1: Core Audio Loop Recording
- [ ] **Retroactive Record Buffer**
  - Always-running circular buffer capturing last N seconds of audio
  - Configurable buffer length (5-30 seconds)
  - One-tap capture to loop memory
  - Visual indicator showing buffer status

- [ ] **Touch Screen Melody Drawing**
  - Draw melodies directly on AMOLED screen
  - X-axis = time position
  - Y-axis = pitch (scale-locked to selected scale)
  - Pentatonic scale as default
  - Support for major, minor, and custom scales

- [ ] **Basic Looping**
  - Seamless loop playback
  - Variable loop length
  - Overdub capability

### Priority 2: Resonator Engine
- [ ] **Resonator/Chord Synth**
  - Triggered by microphone input
  - Scale-locked frequency selection (auto-harmonize input)
  - Multiple resonator modes: string, modal, vocal
  - Adjustable decay time and resonance
  - Velocity-sensitive response to input amplitude

- [ ] **Scale Selection**
  - Built-in scales: pentatonic major/minor, major, minor, blues, dorian, phrygian
  - Root note selection (all 12 notes)
  - IMU-based scale/mode switching (tilt gestures)

### Priority 3: Audio Processing Engines
- [ ] **Rhythmic Chop Engine** (XLN Audio Life-style)
  - Analyze recorded audio for transients
  - Auto-chop into rhythmic patterns
  - Adjustable density and randomness
  - Swing/shuffle control
  - Multiple pattern presets

- [ ] **Simple Synth Engine**
  - Standalone synthesizer not requiring microphone
  - Basic waveforms: sine, saw, square, triangle
  - Scale-locked playback from touch/IMU
  - Simple envelope (ADSR)

- [ ] **Drum Sample Engine**
  - Built-in drum samples (kick, snare, hi-hat, clap, etc.)
  - Simple pattern sequencer (16-step)
  - Tempo control (60-180 BPM)
  - Tap-tempo functionality

- [ ] **Sequencable Effects Chain** (Infiltrator-style)
  - Individual effects that can be sequenced:
    - Filter (LP/HP/BP)
    - Bitcrusher
    - Delay
    - Reverb (simple)
    - Distortion
    - Granular
  - Each effect has parameter that can be sequenced over time
  - Per-step effect bypass

### Priority 4: Input Controls
- [ ] **Touch Screen Interaction**
  - Drawing melodies
  - Parameter adjustment via touch gestures
  - Visual feedback for all operations

- [ ] **IMU/Motion Control**
  - Tilt to control pitch (Y-axis)
  - Tilt to control filter cutoff (X-axis)
  - Shake gesture to trigger effects
  - Custom gesture recognition for advanced users

- [ ] **Physical Buttons (4)**
  - Configurable button mappings
  - Default: Record, Stop, Loop Select, Effect Bypass
  - Support for long-press and double-press

### Priority 5: Storage & Export
- [ ] **SD Card Storage**
  - Save loops as WAV files
  - Export individual stems
  - Project/preset save system
  - Organized folder structure

- [ ] **Cloud Sync**
  - WiFi upload to cloud storage
  - Bluetooth file transfer to mobile
  - Simple companion app for stem management

### Priority 6: UI/UX Features
- [ ] **Display Modes**
  - Waveform visualization
  - Loop status indicators
  - Effect parameter displays
  - Retro/pixel aesthetic

- [ ] **Dead-Front Display**
  - Hidden-until-lit interface
  - AMOLED shines through translucent plastic (0.6-1.0mm)
  - Optional diffusion layer for soft glow

---

## 4. Architecture Requirements

### Modular Codebase Design

The firmware must be architected to allow easy migration from ESP32 prototype to custom PCB.

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐  │
│  │   UI    │ │ Audio   │ │Storage │ │  Connectivity   │  │
│  │ Manager │ │ Manager │ │ Manager│ │    Manager      │  │
│  └────┬────┘ └────┬────┘ └────┬───┘ └────────┬────────┘  │
└───────┼────────────┼───────────┼──────────────┼────────────┘
        │            │           │              │
┌───────┼────────────┼───────────┼──────────────┼────────────┐
│       ▼            ▼           ▼              ▼            │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              Hardware Abstraction Layer (HAL)       │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌──────────┐  │  │
│  │  │Display  │ │  Audio  │ │   IMU   │ │  Storage │  │  │
│  │  │   HAL   │ │   HAL   │ │   HAL   │ │    HAL   │  │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └──────────┘  │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              Platform Specific Layer                │  │
│  │        (ESP32 implementation / Custom PCB)          │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### DSP Architecture (Faust)

All DSP code must be written in Faust for cross-platform portability:

- Faust → C/C++ for ESP32
- Faust → Native code for custom PCB
- Faust → WebAssembly for companion app

---

## 5. Technical Implementation Notes

### Audio Path
```
Mic Input → ADC → DSP Engine 1 (Resonator)
                    DSP Engine 2 (Chopper)
                    DSP Engine 3 (Synth)
                    DSP Engine 4 (Drums)
                    DSP Engine 5 (Effects)
                  ↓
           Mixer → DAC → Speaker/Headphones
```

### Memory Management
- Audio buffers in internal DRAM (DMA-capable)
- UI buffers in PSRAM
- Retroactive buffer: ~30 seconds @ 16kHz = ~480KB (manageable)

### Task Scheduling (FreeRTOS)
- Core 0: UI, Touch, IMU, Display
- Core 1: Audio processing, I2S

---

## 6. Success Criteria

### MVP (Priority 1-2)
- [ ] Retroactive buffer records and captures audio
- [ ] Touch screen draws and plays back melodies
- [ ] Resonator responds to mic input in scale
- [ ] IMU controls filter and pitch

### Beta (Priority 1-4)
- [ ] All DSP engines functional
- [ ] 4 physical buttons working
- [ ] SD card save/load working

### Release (Priority 1-6)
- [ ] Cloud sync functional
- [ ] All effects chain working
- [ ] Production-ready UI
- [ ] Dead-front display aesthetic achieved

---

## 7. References

- **XLN Audio Life** - Rhythmic chopping reference
- **Tim Exile Endless** - Retroactive looping paradigm
- **Devious Machines Infiltrator** - Sequencable effects
- **Terra AI Compass** - Dead-front display technique
- **Teenage Engineering TP-7** - Aesthetic reference
- **Waveshare ESP32-S3-Touch-AMOLED-1.75** - Current prototype board
- **Waveshare WM8960 Audio Board** - Headphone output

---

## 8. Audio Architecture

**See:** `docs/AUDIO_ARCHITECTURE.md` for complete details.

### Current Implementation
- ES8311 codec drives the onboard speaker via PA_EN (GPIO41)
- MCLK generated via LEDC at 12.288 MHz
- I2S_NUM_0 at 16kHz, 16-bit stereo
- ES8311 internal ADC for mic input

### Pending Implementation
- WM8960 Audio Board for headphone output
- Will use I2S_NUM_1 (GPIO 12, 16, 19, 20)
- Shares I2C bus with ES8311 (different address: 0x1A)

### Architecture Decision
For prototype: **Dual codec** (ES8311 speaker + WM8960 headphones)
For production: Consider **WM8960 only** (simpler, better specs)

---

## 9. Open Questions

1. ~~**Exact button pin assignments**~~ → Using PCF8574 I2C GPIO expander at 0x20
2. **SD card file format** - WAV specs (16-bit @ 16kHz confirmed for prototype)
3. **Cloud service** - What backend for WiFi sync?
4. **Companion app** - Native iOS/Android or web-based?
5. **Faust compiler setup** - CI/CD pipeline for Faust → ESP32
6. **Custom PCB timeline** - When to start vs. continue prototyping on Waveshare
7. ~~**Audio codec clarification**~~ → ES8311 for speaker, WM8960 for headphones (see §8)

