# Wingie2 Faust DSP Reference Guide

> Detailed reference for adapting physical modeling resonator techniques to the Tombogo x Rheome Collab Gadget
>
> **Source:** https://github.com/mengqimusic/Wingie2/blob/main/Wingie2.dsp
> **Version:** 3.1 (Meng Qi, 2020-2023)
> **Platform:** ESP32 (Faust → C++)

---

## Overview

Wingie2 is a spectral physical modeling synthesizer that models bells, bars, strings, and various resonators. It uses **modal synthesis** where each note is composed of multiple resonant modes (harmonics) with independent decay times.

### Why This Matters for Your Gadget

| Wingie2 Feature | Gadget Application |
|-----------------|---------------------|
| `pm.modeFilter` resonator | Scale-locked resonator with adjustable harmonics |
| Amplitude follower | Retroactive trigger detection |
| Multiple timbres (poly, strings, bars, cave) | Different resonator character modes |
| Alt tuning system | Scale quantization |
| Wet/dry mixing | Blend raw input with processed output |

---

## 1. Modal Resonator (Core Technique)

### Concept

Modal synthesis models physical objects as a set of resonant modes. Each mode has:
- **Frequency** - The pitch of that harmonic
- **Decay** - How quickly it fades
- **Gain** - Volume of that harmonic

### Wingie2 Implementation

```faust
// ============================================================
// MODAL RESONATOR - Physical modeling filter
// ============================================================

// The main resonator function
r(note, index, source) = pm.modeFilter(a, b, ba.lin2LogGain(c))
with {
    // index selects which harmonic (0-8 for 9 harmonics)
    a = min(f(note, index, source), 16000);  // Frequency (capped at 16kHz)
    b = (env_mode_change * decay) + 0.05;    // Decay time (min 0.05s)
    c = env_mute(button("mute_%index")) *
        (ba.if(a == 16000, 0, 1) : si.smoo); // Gain with mute control
};

// pm.modeFilter arguments:
//   a = frequency (Hz)
//   b = decay time (seconds)
//   c = input signal (gain-controlled)

// ============================================================
// FREQUENCY GENERATION - Multiple timbres
// ============================================================

// Integer ratios (string-like harmonics)
int_ratios(freq, n) = freq * (n + 1);

// Bar ratios (metallic/bell-like) - inharmonic overtones
bar_factor = 0.44444;
bar_ratios(freq, n) = freq * bar_factor * pow((n + 1) + 0.5, 2);

// Fixed frequency resonator (9 static frequencies)
req(n) = 62, 115, 218, 411, 777, 1500, 2800, 5200, 11000
       : ba.selectn(nHarmonics, n);

// Polyphonic mode - 3 notes × 3 harmonics each
poly_norm(n) = a, a * 2, a * 3, b, b * 2, b * 3, c, c * 2, c * 3
             : ba.selectn(nHarmonics, n)
with {
    a = pn0 : mtof;
    b = pn1 : mtof;
    c = pn2 : mtof;
};

// Final frequency selector (0=poly, 1=strings, 2=bars, 3=cave)
f(note, n, s) =
    poly(n),
    strings(note, n),
    bars(note, n),
    cave(n)
    : ba.selectn(4, s);
```

### Key Parameters

```faust
nHarmonics = 9;  // Number of resonant modes per voice
decay = hslider("decay", 5.0, 0.1, 10.0, 0.1);
```

---

## 2. Envelope Generators

### Concept

Wingie2 uses envelopes for:
1. **Mode change** - Smooth transition when switching timbres
2. **Mute** - Per-harmonic muting control
3. **Decay** - Overall amplitude envelope

### Wingie2 Implementation

```faust
// ============================================================
// ENVELOPE GENERATORS
// ============================================================

// Attack-Release envelope for mode changes
// Attack: 2ms, Decay: variable, Release: instant
env_mode_change = 1 - en.ar(0.002, env_mode_change_decay, button("mode_changed"));

// Attack-Sustain-Release envelope for muting
// Attack: 250ms, Sustain: 1.0, Release: 250ms
env_mute(t) = 1 - en.asr(0.25, 1., 0.25, t);

// Parameters
env_mode_change_decay = hslider("env_mode_change_decay", 0.05, 0, 1, 0.01);
decay = hslider("decay", 5.0, 0.1, 10.0, 0.1);  // Main decay time
```

### Key Faust Libraries

| Function | Library | Purpose |
|----------|---------|---------|
| `en.ar(at, rt, trig)` | stdfaust.lib | Attack-Release envelope |
| `en.asr(at, rt, kt, trig)` | stdfaust.lib | Attack-Sustain-Release envelope |
| `si.smoo` | stdfaust.lib | Smooth parameter changes (anti-pop) |
| `ba.lin2LogGain` | stdfaust.lib | Linear-to-dB conversion |

---

## 3. Input Processing

### Concept

Before hitting the resonators, audio is processed to:
1. Remove DC offset (prevents clicks/pops)
2. Filter out high-frequency content (anti-aliasing)
3. Detect amplitude for triggering

### Wingie2 Implementation

```faust
// ============================================================
// INPUT PROCESSING
// ============================================================

// Remove DC offset from input
fi.dcblocker

// Pre-resonator lowpass (removes ultrasonic)
fi.lowpass(1, 4000)  // First-order, 4kHz cutoff

// Amplitude follower for trigger detection
an.amp_follower(amp_follower_decay)

// Threshold comparison (outputs 0 or 1)
: _ > left_thresh : hbargraph("left_trig", 0, 1)

// Parameters
amp_follower_decay = hslider("amp_follower_decay", 0.025, 0.001, 0.1, 0.001);
left_thresh = hslider("left_thresh", 0.1, 0, 1, 0.01);
right_thresh = hslider("right_thresh", 0.1, 0, 1, 0.01);
```

### Key Faust Filters

| Filter | Syntax | Use Case |
|--------|--------|----------|
| DC blocker | `fi.dcblocker` | Remove DC offset |
| Lowpass | `fi.lowpass(order, freq)` | Anti-aliasing, tone control |
| Highpass | `fi.highpass(order, freq)` | Bass cut |
| Bandpass | `fi.bandpass(order, freq, q)` | Resonant filtering |
| Peaking EQ | `fi.peaking(eq_freq, eq_q, eq_gain)` | Boost/cut frequencies |

---

## 4. Waveshaping / Distortion

### Concept

After resonators, signal can be waveshaped for:
1. Added harmonic content
2. Soft clipping (tape-like saturation)
3. Hard limiting

### Wingie2 Implementation

```faust
// ============================================================
// WAVESHAPER / DISTORTION
// ============================================================

// Cubic non-linear transfer function
// Args: pre-gain, post-gain
ef.cubicnl(0.01, 0)

// Also available (commented in source):
// aa.cubic1    - Alternative waveshaper
// co.limiter_1176_R4_mono - Compressor/limiter
```

### Faust Effects Library Reference

| Effect | Syntax | Description |
|--------|--------|-------------|
| Cubic distortion | `ef.cubicnl(pre, post)` | Polynomial waveshaping |
| Soft clipper | `ef.softclipper` | Soft saturation |
| Hard clipper | `ef.hardclipper` | Hard limiting |
| Tanh distortion | `ef.tanh` | Tanh-based saturation |
| Sine shaper | `ef.sine` | Sine-based waveshaping |

---

## 5. Complete Signal Flow

### Wingie2 Process Block

```faust
// ============================================================
// COMPLETE SIGNAL CHAIN
// ============================================================

process = _,_                                           // Stereo input
    : fi.dcblocker, fi.dcblocker                        // DC removal


    : (_ // LEFT CHANNEL <: attach(_,                                 // Keep input for output
        _ : an.amp_follower(amp_follower_decay)        // Amplitude follower
        : _ > left_thresh                              // Threshold
        : hbargraph("left_trig", 0, 1))),             // Output trigger

    // RIGHT CHANNEL (same)
    (_ <: attach(_,
        _ : an.amp_follower(amp_follower_decay)
        : _ > right_thresh
        : hbargraph("right_trig", 0, 1)))

    // Apply volume envelopes
    : (_ * env_mode_change * volume0),
      (_ * env_mode_change * volume1)

    // Split to resonators + dry path
    <:

    // Resonator path
    (_ * resonator_input_gain
        : fi.lowpass(1, 4000)                         // Pre-filter
        <: hgroup("left",                              // Left resonators
            sum(i, nHarmonics, r(note0, i, mode0)))    // 9 modes
        * pre_clip_gain),                              // Pre-waveshaper gain

    (_ * resonator_input_gain
        : fi.lowpass(1, 4000)
        <: hgroup("right",                             // Right resonators
            sum(i, nHarmonics, r(note1, i, mode1)))    // 9 modes
        * pre_clip_gain),

    // Dry path (passthrough)
    _, _

    // Waveshaping
    : ef.cubicnl(0.01, 0), ef.cubicnl(0.01, 0), _, _

    // Wet/dry mixing
    : _ * vol_wet0 * post_clip_gain,                  // Wet left
      _ * vol_wet1 * post_clip_gain,                  // Wet right
      _ * vol_dry0,                                    // Dry left
      _ * vol_dry1                                     // Dry right

    // Final output
    :>(_ * output_gain), (_ * output_gain);           // Stereo output
```

### Signal Flow Diagram

```
                        ┌─────────────────────────────────────┐
                        │           INPUT (L/R)               │
                        └──────────────┬──────────────────────┘
                                       │
                        ┌──────────────▼──────────────────────┐
                        │         DC BLOCKER                  │
                        └──────────────┬──────────────────────┘
                                       │
              ┌────────────────────────┼────────────────────────┐
              │                        │                        │
              ▼                        ▼                        ▼
    ┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
    │ AMP FOLLOWER L │      │ AMP FOLLOWER R │      │   DRY PATH      │
    │ + THRESHOLD    │      │ + THRESHOLD    │      │   (passthrough) │
    └────────┬────────┘      └────────┬────────┘      └────────┬────────┘
             │                        │                        │
             └────────────────────────┼────────────────────────┘
                                      │
                        ┌──────────────▼──────────────────────┐
                        │        VOLUME ENVELOPE               │
                        │      (mode change smoothing)         │
                        └──────────────┬──────────────────────┘
                                       │
              ┌────────────────────────┼────────────────────────┐
              │                        │                        │
              ▼                        ▼                        │
    ┌─────────────────┐      ┌─────────────────┐                 │
    │   LOWPASS       │      │   LOWPASS       │                 │
    │   4000Hz        │      │   4000Hz        │                 │
    └────────┬────────┘      └────────┬────────┘                 │
             │                        │                          │
             ▼                        ▼                          │
    ┌─────────────────┐      ┌─────────────────┐                │
    │ 9x MODE FILTER  │      │ 9x MODE FILTER  │                │
    │ (resonator)     │      │ (resonator)     │                │
    └────────┬────────┘      └────────┬────────┘                │
             │                        │                          │
             └────────────────────────┼────────────────────────┘
                                      │
                        ┌──────────────▼──────────────────────┐
                        │       CUBIC WAVESHAPER              │
                        └──────────────┬──────────────────────┘
                                       │
              ┌────────────────────────┼────────────────────────┐
              │                        │                        │
              ▼                        ▼                        │
    ┌─────────────────┐      ┌─────────────────┐                │
    │  WET MIX        │      │  DRY MIX        │                │
    └────────┬────────┘      └────────┬────────┘                │
             │                        │                          │
             └────────────────────────┼────────────────────────┘
                                      │
                        ┌──────────────▼──────────────────────┐
                        │         OUTPUT GAIN                  │
                        └──────────────────────────────────────┘
```

---

## 6. Scale Quantization / Alternate Tuning

### Concept

Wingie2 supports alternate tunings via ratio sliders - useful for microtonal scales.

### Wingie2 Implementation

```faust
// ============================================================
// ALTERNATE TUNING SYSTEM
// ============================================================

// Enable alternate tuning
use_alt_tuning = button("../../use_alt_tuning");

// 12 ratio sliders (one per semitone)
alt_tuning_ratios = (
    hslider("../../alt_tuning_ratio_0", 1.0, 1.0, 2.0, 0.001),  // C
    hslider("../../alt_tuning_ratio_1", 1.0, 1.0, 2.0, 0.001),  // C#
    hslider("../../alt_tuning_ratio_2", 1.0, 1.0, 2.0, 0.001),  // D
    hslider("../../alt_tuning_ratio_3", 1.0, 1.0, 2.0, 0.001),  // D#
    hslider("../../alt_tuning_ratio_4", 1.0, 1.0, 2.0, 0.001),  // E
    hslider("../../alt_tuning_ratio_5", 1.0, 1.0, 2.0, 0.001),  // F
    hslider("../../alt_tuning_ratio_6", 1.0, 1.0, 2.0, 0.001),  // F#
    hslider("../../alt_tuning_ratio_7", 1.0, 1.0, 2.0, 0.001),  // G
    hslider("../../alt_tuning_ratio_8", 1.0, 1.0, 2.0, 0.001),  // G#
    hslider("../../alt_tuning_ratio_9", 1.0, 1.0, 2.0, 0.001),  // A
    hslider("../../alt_tuning_ratio_10", 1.0, 1.0, 2.0, 0.001), // A#
    hslider("../../alt_tuning_ratio_11", 1.0, 1.0, 2.0, 0.001)  // B
);

// Reference pitch (A4)
a3_freq = hslider("../../a3_freq", 440, 300, 600, 0.01);

// MIDI to frequency (standard 12-TET)
mtof(note) = a3_freq * pow(2., (note - 69) / 12);

// MIDI to quantized frequency (alternate tuning)
mtoq(note) = f with {
    n = note % 12;           // Scale degree (0-11)
    c = note - n;            // C note in given octave
    f = mtof(c) * (alt_tuning_ratios : ba.selectn(12, n));
};
```

### Simplified Scale Quantization (For Gadget)

```faust
// ============================================================
// SIMPLIFIED SCALE QUANTIZATION
// ============================================================

// Pre-defined scale ratios (modify as needed)
scale_type = hslider("scale", 0, 0, 3, 1);  // 0=chromatic, 1=major, 2=minor, 3=pentatonic

// Scale ratio arrays (as tuples)
major_ratios = (1.0, 9.0/8.0, 5.0/4.0, 4.0/3.0, 3.0/2.0, 5.0/3.0, 15.0/8.0, 2.0);
minor_ratios = (1.0, 9.0/8.0, 6.0/5.0, 4.0/3.0, 3.0/2.0, 8.0/5.0, 9.0/4.0, 2.0);
pentatonic_ratios = (1.0, 9.0/8.0, 3.0/2.0, 5.0/3.0, 2.0);

// Select scale based on parameter
selected_ratios = major_ratios, minor_ratios, pentatonic_ratios : ba.selectn(3, scale_type);

// Quantize frequency to nearest scale degree
quantize_to_scale(freq) = freq * selected_ratios : ba.search(12, (freq * selected_ratios) : ba.min)
```

---

## 7. Parameter Smoothing

### Concept

All parameters should be smoothed to prevent clicks/pops when values change abruptly.

### Wingie2 Implementation

```faust
// ============================================================
// PARAMETER SMOOTHING
// ============================================================

// All parameters use si.smoo for smooth transitions
volume0 = hslider("volume0", 0.25, 0, 1, 0.01) : ba.lin2LogGain : si.smoo;
mix0 = hslider("mix0", 1, 0, 1, 0.01) : si.smoo;
mode0 = hslider("mode0", 0, 0, 4, 1);  // Discrete, no smoothing needed

// si.smoo is equivalent to: si.pole(z-1) for smooth interpolation
```

---

## 8. Minimal Faust Resonator Template

### Ready-to-Compile Foundation

```faust
// ============================================================
// GADGET RESONATOR - Minimal Template
// ============================================================

declare name "GadgetResonator";
declare version "1.0";
declare author "Tombogo x Rheome";
declare license "MIT";

import("stdfaust.lib");

// ============== PARAMETERS ==============

// Core resonator parameters
decay = hslider("decay", 2.0, 0.1, 10.0, 0.01) : si.smoo;
gain = hslider("gain", 0.5, 0, 1, 0.01) : si.smoo;
base_freq = hslider("freq", 440, 100, 2000, 1);

// Input processing
input_gain = hslider("input_gain", 0.5, 0, 1, 0.01) : si.smoo;

// Timbre selection
timbre = hslider("timbre", 0, 0, 2, 1);  // 0=harmonic, 1=inharmonic, 2=fixed

// ============== HARMONIC GENERATION ==============

nHarmonics = 6;

// Harmonic series (string-like)
harmonic_freq(n) = base_freq * (n + 1);

// Inharmonic (bell-like)
bar_factor = 0.44444;
inharmonic_freq(n) = base_freq * bar_factor * pow((n + 1) + 0.5, 2);

// Fixed frequencies (metallic)
fixed_freq(n) = ba.selectn(nHarmonics, n)
with {
    sel = (62, 115, 218, 411, 777, 1500);
};

// Select based on timbre
mode_freq(n) = harmonic_freq(n), inharmonic_freq(n), fixed_freq(n) : ba.selectn(3, timbre);

// ============== RESONATOR ==============

// Single mode filter
resonator_mode(n) = pm.modeFilter(mode_freq(n), decay, gain);

// Sum all modes in parallel
resonators = par(i, nHarmonics, resonator_mode(i));

// ============== MAIN PROCESS ==============

process = _
    : fi.dcblocker                           // Remove DC
    : fi.lowpass(1, 8000)                    // Anti-alias filter
    : * (input_gain)                         // Input gain
    : resonators                              // Through modal filters
    : fi.dcblocker                            // Clean output
    : * (gain);                              // Output gain
```

---

## 9. Key Libraries Reference

### Essential Faust Libraries

| Library | Prefix | Contents |
|---------|--------|----------|
| stdfaust.lib | (none) | Standard functions |
| filter.lib | fi.* | Filters (lowpass, highpass, etc.) |
| phaser.lib | ph.* | Phaser effects |
| reverb.lib | re.* | Reverb algorithms |
| delay.lib | de.* | Delay lines |
| envelope.lib | en.* | Envelope generators |
| noise.lib | no.* | Noise generators |
| oscillator.lib | os.* | Basic oscillators |
| piano.lib | pf.* | Piano modeling |
| physical.lib | pm.* | Physical modeling |
| analyze.lib | an.* | Analyzers (FFT, follower) |
| effect.lib | ef.* | Effects (distortion, etc.) |
| compressor.lib | co.* | Dynamics processing |
| eq.lib | eq.* | Equalizers |
| misceffect.lib | aa.* | Additional effects |

---

## 10. Compilation & ESP32 Integration

### The Faust Compilation Workflow

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  resonator  │───▶│    faust    │───▶│   resonator │───▶│   Arduino   │
│    .dsp     │    │  compiler   │    │    .cpp     │    │   project   │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
```

### Basic Compilation

```bash
# Basic compilation to C++
faust -cn resonator -i -a cpp resonator.dsp > resonator.cpp

# With floating-point (faster on ESP32)
faust -cn resonator -i -a cpp -float resonator.dsp > resonator.cpp
```

### ESP32-Specific Compilation

Wingie2 uses this exact command:

```bash
# Wingie2's compilation method
faust2esp32 -ac101 -lib Wingie2.dsp
```

### faust2esp32 Options

```bash
# Common options
faust2esp32 [options] your_dsp_file.dsp

# Key options:
#   -a <arch>     Architecture file (cpp, wasm, etc.)
#   -c <num>      Number of channels (default: 2)
#   -dsp          Add DSP as global variable
#   -e            Expand macros
#   -f            Force recompilation
#   -i            Interactive mode (UI)
#   -l            Use local copy of libraries
#   -lib          Create static library
#   -mcu          Optimize for microcontroller
#   -n <name>     Class name
#   -oc <opt>     Optimization level (0-3)
#   -os           Single-channel (mono)
#   -ot           Double precision (slower)
#   -q            Quiet mode
#   -s            SD card support
#   -u            Generate UI
#   -x            XCode project
```

### Recommended Compilation for Gadget

```bash
# For ESP32 with I2S audio
faust -cn Resonator \
    -i \
    -a cpp \
    -float \
    -dsp \
    resonator.dsp > resonator.cpp

# Or using faust2esp32 directly
faust2esp32 -ac101 -lib -n Resonator resonator.dsp
```

### Key Compiler Flags

| Flag | Purpose |
|------|---------|
| `-cn <name>` | Class name for generated code |
| `-i` | Generate interleaved code (required for I2S) |
| `-a <arch>` | Target architecture (cpp, wasm, etc.) |
| `-float` | Use float (faster on ESP32) |
| `-double` | Use double precision (slower but more accurate) |
| `-dsp` | Add DSP as global variable for UI access |
| `-e` | Expand macros (larger but faster) |

### Integration Steps

1. **Compile DSP to C++**
   ```bash
   faust -cn Resonator -i -a cpp -float resonator.dsp > resonator.cpp
   ```

2. **Add to PlatformIO project**
   ```
   firmware/
   ├── src/
   │   ├── main.cpp
   │   └── Resonator.cpp    # Generated
   ```

3. **Include in your code**
   ```cpp
   #include "Resonator.cpp"

   Resonator* gResonator;

   void setup() {
       gResonator = new Resonator();
       gResonator->init(16000);  // Sample rate
   }

   void audioCallback(float* input, float* output, int frames) {
       gResonator->compute(frames, input, output);
   }
   ```

### Architecture File for ESP32

Wingie2 uses `-ac101` which references the ESP32 I2S architecture. This file handles:
- I2S DMA buffer management
- Sample rate configuration
- Stereo/mono routing

Your gadget uses ES8311 codec - you'll want a similar architecture file or modify the generated code to route through your audio HAL.

---

## Summary

The Wingie2 DSP provides excellent reference for:

1. **Modal synthesis** - Using `pm.modeFilter` for physical modeling resonators
2. **Harmonic structuring** - Integer vs inharmonic ratios for different timbres
3. **Envelope design** - AR/ASR for smooth parameter transitions
4. **Signal flow** - DC block → filter → resonator → waveshape → mix → output
5. **Trigger detection** - Amplitude follower for retroactive playing
6. **Scale locking** - Alternate tuning system for microtonal control

The core insight is that **modal synthesis** is the heart of the resonator - each harmonic is an independent resonant filter with its own frequency and decay. This creates the organic, metallic, or string-like qualities that make physical modeling so expressive.

### Recommended Implementation Order

1. **Phase 1**: Compile minimal template, verify audio output
2. **Phase 2**: Add `pm.modeFilter` resonator, test decay
3. **Phase 3**: Add DC blocker + lowpass input processing
4. **Phase 4**: Add amplitude follower for triggers
5. **Phase 5**: Add timbre selection (harmonic/inharmonic/fixed)
6. **Phase 6**: Add scale quantization
7. **Phase 7**: Add waveshaper and wet/dry mix
