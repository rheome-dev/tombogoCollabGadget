# Faust DSP Integration Strategy

## Why Faust?

**Faust (Functional Audio Stream)** is a programming language for digital signal processing that compiles to highly efficient C/C++ code. This allows the same DSP algorithms to be used on:

1. **ESP32** - Compiled to C for real-time audio processing
2. **Custom PCB** - Native C/C++ with optimized code
3. **Web/Companion App** - WebAssembly for browser-based preview
4. **Desktop** - Standalone VST/AU plugins for production

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Faust Source Code                        │
│  (dsp/resonator.dsp, dsp/chopper.dsp, etc.)              │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Faust Compiler                          │
│  faust -cn resonator -i -a cpp resonator.dsp > resonator.cpp│
└─────────────────────────────────────────────────────────────┘
                              │
            ┌─────────────────┼─────────────────┐
            ▼                 ▼                 ▼
    ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
    │   ESP32       │  │  Custom PCB  │  │   Web/WASM   │
    │   (C++)       │  │   (C++)      │  │  (WebASM)    │
    └──────────────┘  └──────────────┘  └──────────────┘
```

## DSP Modules to Implement in Faust

### 1. Resonator (`resonator.dsp`)

```faust
// Pseudocode - actual implementation needed
process = _ <: resonator with {
    resonator = bandpass : *;
};
```

**Features:**
- Multi-resonator bank (8-16 resonators)
- Scale locking (pre-computed frequency table)
- Input following (velocity-sensitive)
- Decay envelope

### 2. Rhythmic Chopper (`chopper.dsp`)

```faust
// Features needed:
- Transient detection
- Sample slicing
- Pattern generation
- Swing/shuffle
- Density control
```

### 3. Simple Synth (`synth.dsp`)

```faust
// Features:
- Oscillators: sine, saw, square, triangle
- ADSR envelope
- Scale-locked frequency input
- Pitch bend
```

### 4. Drum Sampler (`drums.dsp`)

```faust
// Features:
- Sample playback (pre-loaded)
- 16-step sequencer
- Tempo sync
- Multiple drum sounds
```

### 5. Effects Chain (`effects.dsp`)

```faust
// Each effect as separate module:
- filter: lp, hp, bp with resonance
- bitcrusher: bit depth, sample rate reduction
- delay: time, feedback, mix
- reverb: simple algorithmic
- distortion: waveshaping
- granular: window size, density
```

## Compilation Pipeline

### ESP32

```bash
# Compile Faust to C with ESP32 optimizations
faust -cn resonator -i -a esp32.cpp resonator.dsp > resonator.cpp

# Alternative: Generate standalone C
faust -cn resonator -i resonator.dsp > resonator.c
```

### WebAssembly

```bash
# Install faust2wasm
# Compile to WASM
faust -cn mydsp -i -a wasm-httpd.cpp mydsp.dsp -o mydsp.wasm
```

## Parameter Control

Faust generates both `compute()` for audio and UI controls. We'll map:

| DSP Parameter | Control Source |
|--------------|----------------|
| Filter Cutoff | IMU X-axis |
| Pitch Bend | IMU Y-axis |
| Decay | Touch Y position |
| Pattern Select | Physical buttons |
| Tempo | Tap tempo / touch |

## Integration with Arduino/ESP-IDF

1. **Generate C code** from Faust
2. **Include in Arduino project** as additional .cpp files
3. **Call compute()** from audio ISR/task
4. **Map parameters** via setter functions

## Development Workflow

1. **Prototype in Faust** - Test DSP algorithms standalone
2. **Compile to Web** - Quick preview in browser
3. **Compile to ESP32** - Real-time testing on hardware
4. **Tune parameters** - Adjust based on hardware performance

## Tools & Setup

### Install Faust

```bash
# macOS
brew install faust

# Linux
apt-get install faust

# Build from source
git clone https://github.com/grame-cncm/faust.git
cd faust
make
```

### VS Code Extensions

- **Faust** - Syntax highlighting
- **Faust LSP** - Autocomplete

## Performance Targets

| Metric | Target |
|--------|--------|
| Sample Rate | 16kHz (ESP32) / 44.1kHz (custom PCB) |
| Latency | < 10ms |
| CPU (ESP32) | < 30% per engine |
| Memory | < 200KB total DSP state |

## Reference Examples

- `faust/examples/` - Standard library
- `faust/tests/` - DSP tests
- Web IDE: https://fausteditor.grame.fr/

## Next Steps

1. Install Faust
2. Create `resonator.dsp` as first DSP module
3. Compile and test on ESP32
4. Build out remaining DSP modules
5. Create build scripts for CI/CD

