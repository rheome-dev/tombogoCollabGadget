# Firmware Port Plan: rhiqoGoMobilePrototype → ESP32-S3

Porting the web-based rhiqoGoMobilePrototype looper/resonator to run natively on the Waveshare ESP32-S3-Touch-AMOLED-1.75 with MCP23017 physical controls.

## Current State Summary

**What works on hardware:**
- ES8311 DAC (speaker out) + ES7210 ADC (mic in) at 16kHz/16-bit stereo
- MCLK generation (12.288 MHz via LEDC on GPIO 42)
- PA amplifier control (GPIO 46) — currently too loud in headphones, needs attenuation
- MCP23017 on Wire2 (GPIO 16/17, addr 0x27) — interrupt-driven input reading
- QSPI AMOLED display (466×466, CO5300 driver via Arduino_GFX)
- LVGL 8.4 display driver (basic status label only)
- Touch input (CST9217), IMU (QMI8658), SD card (SPI)
- Retroactive circular buffer (10s @ 16kHz stereo = 1.28 MB, DRAM fallback)

**What's stubbed:**
- All DSP engines (resonator, chopper, synth, drums, effects) are pass-through
- Encoder software (hardware connected to MCP23017 GPB0-2)
- Full LVGL UI (only status label)
- FreeRTOS tasks (declared but not spawned, running bare loop)

**Critical constraint — PSRAM (RESOLVED):**
The board uses ESP32-S3R8 (8MB OPI PSRAM) which has its own dedicated Octal SPI bus, separate from the display QSPI. The `-mfix-esp32-psram-cache-issue` flag in platformio.ini was for ESP32 (not S3) and incorrectly included. Enable PSRAM with the correct OPI mode setting. If PSRAM enables, buffers can expand dramatically (retroactive buffer to 30s, full-screen canvas, Faust DSP state). The `-mfix` flag was removed by prior work; verify `board_build.psram = opi` is set in platformio.ini.

**Audio path — mono for now:**
16kHz mono processing internally. I2S is configured stereo (L+R identical). Future: record stereo from dual mics to SD card.

## Architecture Decisions

### Audio Sample Rate: 16 kHz (keep as-is)
The web prototype runs at 44.1 kHz but the ESP32 firmware is configured for 16 kHz. At 16 kHz we get:
- Smaller buffers (critical with no PSRAM)
- Less CPU per DSP frame
- 8 kHz Nyquist is adequate for looper/resonator character (lo-fi is the aesthetic)
- Faust DSP compiles for any sample rate

### Buffer Size Strategy (with PSRAM)
ESP32-S3R8 has 8MB OPI PSRAM available. Use PSRAM for large buffers, DRAM for latency-sensitive paths.

| Buffer | Size | Location | Notes |
|--------|------|----------|-------|
| Retroactive circular buffer | 960 KB | PSRAM | 30 sec @ 16kHz mono |
| Captured loop buffer | 960 KB | PSRAM | 30 sec max loop |
| LVGL draw buffers (2×) | ~870 KB | PSRAM | Full 466×466 canvas |
| Audio DMA (I2S) | 4 KB | DMA SRAM | 8 × 256 samples × 2 bytes |
| DSP frame scratch | 4 KB | DRAM | Working buffers for Faust float conversion |
| Slice metadata | 2 KB | DRAM | Up to 32 slices |
| Faust DSP state | ~200 KB | PSRAM | Resonator bands + Freeverb state |
| **Total** | **~3 MB** | | Fits easily in 8MB PSRAM |

**Trade-off**: 2-second retroactive buffer vs web's 30 seconds. This is a significant UX change but unavoidable without PSRAM. The lo-fi constraint actually fits the product character. If PSRAM is eventually resolved, increase to 10+ seconds.

### Mono Audio Path
- Internal processing: mono int16_t throughout
- I2S output: duplicate mono to L+R channels at the final write stage
- Mic input: read one channel only (or average L+R from ES7210)
- Future: write stereo WAV to SD card using both mic channels

### FreeRTOS Task Structure
```
Core 1 (audio, highest priority):
  AudioTask — 1ms period, reads mic DMA, runs DSP chain, writes speaker DMA

Core 0 (everything else):
  InputTask  — 10ms period, polls MCP23017 (interrupt-flagged), debounce, encoder decode
  UITask     — 16ms period (~60fps), LVGL tick + render
  MainLoop   — state machine, event dispatch, SD card I/O
```

### Control Mapping (MCP23017 → Web Prototype)

**Verified from working mcp_test firmware (addr 0x27):**

| MCP Pin | Physical Control | Web Equivalent | Function |
|---------|-----------------|----------------|----------|
| PA0 | Encoder switch | Encoder push | Context-dependent toggle (chop on/off, resonator on/off) |
| PA1 | Encoder B | Encoder turn | Quadrature channel B |
| PA2 | Encoder A | Encoder turn | Quadrature channel A — decode PA1+PA2 for direction+delta |
| PA3 | Button 1 (top) | Capture button | Capture / confirm / clear |
| PA4 | Button 2 (mid) | Shift button | Modifier / latch toggle |
| PA5 | Button 3 (bottom) | Btn3 | Mode switch (chord mode A/B) |
| PB0-PB4 | Joystick 5-way | Joystick | Up/Down/Left/Right/Center — stage navigation + params |
| PB5-PB7 | Spare | — | Available for expansion |

**Verified pin mapping (from hardware test):**

| MCP Pin | Physical Control | Web Equivalent | Function |
|---------|-----------------|---------------|----------|
| PA0 | Encoder switch | Encoder push | Context-dependent toggle |
| PA1 | Encoder B | Encoder turn | Quadrature channel B |
| PA2 | Encoder A | Encoder turn | Quadrature channel A |
| PA3 | Button 1 (top) | Capture button | Capture / confirm / clear |
| PA4 | Button 2 (middle) | Shift button | Modifier / latch toggle |
| PA5 | Button 3 (bottom) | Btn3 | Mode switch (chord mode A/B) |
| PB3 | Joystick A (Up) | Joystick Up | Stage navigation + params |
| PB4 | Joystick B (Left) | Joystick Left | Stage navigation |
| PB5 | Joystick C (Down) | Joystick Down | Stage navigation + params |
| PB6 | Joystick Center | Joystick Center | Pattern randomize / action |
| PB7 | Joystick D (Right) | Joystick Right | Stage navigation |

---

## Implementation Phases

### Phase 0: Headphone Volume Fix + PSRAM Enable + Memory Audit
**Goal:** Fix deafening headphone volume (software attenuation), enable PSRAM, establish memory budget.
**Depends on:** Nothing (do first).

**Tasks:**

1. **Headphone volume attenuation** — in `audio_esp32.cpp`, reduce ES8311 DAC volume at init. The NS4150B amp is designed for speaker and routes directly to headphones via PJ307 switched jack. The ES8311 DAC has a volume control register (0x32) and a DAC output volume register (0x22). Start with ES8311_DACVolume at ~30% (try register 0x22 value 0x1F instead of default 0xC0). Also try reducing the PA gain by lowering the PWM duty cycle on GPIO 46 or by adding a pre-gain multiplier in software before I2S write.

2. **Enable PSRAM** — check that platformio.ini has `board_build.psram = opi` (for ESP32-S3R8 OPI mode). The prior fix only commented out flags; verify PSRAM actually allocates with `ESP.getPsramSize() > 0`. If PSRAM is confirmed working, all large buffers (retroactive buffer, LVGL canvas, Faust DSP state) go to PSRAM via `ps_malloc()` / `heap_caps_malloc(MALLOC_CAP_SPIRAM)`. If PSRAM still doesn't work, fall back to Phase 0 DRAM plan from v1 of this document.

3. **Memory audit** — add `ESP.getFreeHeap()` and `ESP.getFreePsram()` logging after each init stage in main.cpp: Wire init, display init, MCP init, audio init, LVGL init, buffer allocation. This tells us actual headroom for Phase 6 Faust DSP memory budget.

**Files to modify:**
- `firmware/platforms/esp32/audio_esp32.cpp` — ES8311 volume register writes
- `firmware/src/config.h` — PSRAM_AVAILABLE, buffer size constants
- `firmware/src/main.cpp` — heap + psram logging

---

### Phase 1: MCP23017 Input Driver + Encoder Decoding
**Goal:** Replace raw register reads with a proper input system supporting debounce, multi-click, encoder quadrature, and shift state.
**Depends on:** Phase 0 (need heap budget to size event queue).

**Context from web prototype (controls-ui.js):**
- Buttons need: single-click, double-click (300ms window), long-press (500ms)
- Shift button: tap toggles, double-tap latches
- Encoder: quadrature decode from PA1 (B) + PA2 (A), push from PA0
- Joystick: 5-way discrete (not analog), active-low

**Tasks:**
1. Create `firmware/src/input/mcp_input.h` — input event types and API:
   ```cpp
   enum InputEvent {
       EVT_BUTTON_PRESS, EVT_BUTTON_RELEASE, EVT_BUTTON_DOUBLE,
       EVT_BUTTON_LONG, EVT_ENCODER_CW, EVT_ENCODER_CCW,
       EVT_ENCODER_PUSH, EVT_JOY_UP, EVT_JOY_DOWN,
       EVT_JOY_LEFT, EVT_JOY_RIGHT, EVT_JOY_CENTER
   };
   struct InputMsg { InputEvent type; uint8_t id; uint32_t timestamp; };
   ```

2. Create `firmware/src/input/mcp_input.cpp`:
   - Init Wire2, configure MCP23017 (all inputs, pull-ups, INTA on GPIO 18)
   - ISR sets flag; `MCPInput_poll()` reads GPIO AB, detects changes
   - Encoder quadrature: on PA2 (CLK) falling edge, read PA1 (DT) for direction
   - Button debounce: 50ms minimum between state changes per pin
   - Multi-click detection: 300ms window after first release
   - Long-press: fire after 500ms hold
   - Shift state: track PA4 for modifier, latch on double-tap
   - Push events into a FreeRTOS queue (xQueueSend) for main loop consumption

3. Create `firmware/src/input/input_task.cpp`:
   - FreeRTOS task on Core 0, 10ms period
   - Calls `MCPInput_poll()` each tick
   - Manages timing for debounce/multi-click/long-press state machines

4. Integrate into main.cpp:
   - Replace raw MCP reads with `MCPInput_init()` + task creation
   - Main loop reads event queue with `xQueueReceive()`

**Key difference from web:** The web prototype processes encoder as a continuous 0-1 value from drag gestures. The hardware encoder produces discrete CW/CCW clicks. Map encoder clicks to parameter deltas (e.g., ±0.05 for density, ±1 semitone for pitch).

**Files to create:**
- `firmware/src/input/mcp_input.h`
- `firmware/src/input/mcp_input.cpp`
- `firmware/src/input/input_task.h`
- `firmware/src/input/input_task.cpp`

**Files to modify:**
- `firmware/src/main.cpp` — init + task creation
- `platformio.ini` — add src_filter if needed for new directory

---

### Phase 2: State Machine + Event Dispatch
**Goal:** Port the web prototype's stage-based state machine (IDLE → CAPTURE_REVIEW → CHOP → RESONATE) to C++.
**Depends on:** Phase 1 (needs input events).

**Context from web prototype (app.js):**
The web app has 5 stages with strict transitions:
```
IDLE → CAPTURE_REVIEW → CHOP ↔ RESONATE → CHORD_LOOP (future)
```
Each stage defines unique behavior for encoder, joystick, and buttons.

**Tasks:**
1. Create `firmware/src/core/stage_manager.h`:
   ```cpp
   enum Stage { STAGE_IDLE, STAGE_CAPTURE_REVIEW, STAGE_CHOP, STAGE_RESONATE };

   void StageManager_init();
   Stage StageManager_current();
   void StageManager_handleInput(InputMsg msg);
   void StageManager_update();  // Called each main loop tick
   ```

2. Create `firmware/src/core/stage_manager.cpp`:
   - Stage transition logic mirroring app.js:
     - IDLE: Capture button → start recording / trigger capture → CAPTURE_REVIEW
     - CAPTURE_REVIEW: Joy right → CHOP, Joy up/down → cycle bar length, Encoder → pitch
     - CHOP: Encoder push → toggle chop on/off, Joy center → randomize pattern, Joy right → RESONATE, Joy left → CAPTURE_REVIEW
     - RESONATE: Encoder push → toggle resonator, Joy up/down → chord select, Encoder turn → wet/dry mix, Joy left → CHOP
   - Each stage has `enter()` / `exit()` / `handleInput()` / `update()` methods
   - Shift+Capture → reset to IDLE from any stage

3. Wire into main.cpp:
   - Main loop: read input queue → `StageManager_handleInput()` → `StageManager_update()`
   - Stage changes trigger audio engine reconfiguration and UI updates

**Mapping from web controls to hardware:**

| Web Action | Hardware Equivalent |
|-----------|-------------------|
| Capture button single-click | PA3 single press |
| Shift + Capture | PA4 held + PA3 press |
| Encoder push | PA0 press |
| Encoder turn (drag up/down) | PA1+PA2 quadrature CW/CCW |
| Joystick 5-way | PB0-PB4 directions |
| Btn3 (chord mode) | PA5 press |

**Files to create:**
- `firmware/src/core/stage_manager.h`
- `firmware/src/core/stage_manager.cpp`

**Files to modify:**
- `firmware/src/main.cpp`

---

### Phase 3: Audio Pipeline — Retroactive Capture + Loop Playback
**Goal:** Get the core audio loop working: always-recording circular buffer → capture → loop playback with mixing.
**Depends on:** Phase 0 (buffer sizes), Phase 2 (stage triggers capture).

**Context:**
The existing `retroactive_buffer.cpp` already implements circular recording and capture. The web prototype's `circular-buffer-worklet.js` does the same thing. The firmware version needs:
- Verified mono operation
- Correct buffer sizing for DRAM
- Integration with the state machine

**Tasks:**
1. Modify `firmware/src/core/retroactive_buffer.cpp`:
   - Change to mono operation (single int16_t channel, not stereo interleaved)
   - Buffer size: `SAMPLE_RATE * RETROACTIVE_BUFFER_SEC` samples (e.g., 16000 × 2 = 32000 samples = 64 KB)
   - `capture()`: copy the last N samples to a separate loop buffer
   - `getLoopBuffer()` / `getLoopLength()`: accessors for playback

2. Modify `firmware/src/core/audio_engine.cpp`:
   - Restructure `AudioEngine_process()` for the full signal chain:
     ```
     Mic (mono from I2S) → retroactive_buffer.write()
                         → [if CAPTURE_REVIEW+] loop playback (read from captured buffer, wrap)
                         → [if CHOP active] chop engine processes loop
                         → [if RESONATE active] resonator processes output
                         → mono → duplicate to stereo → I2S write
     ```
   - Loop playback: simple read pointer advancing through captured buffer, wrapping at end
   - Mix control: when in CAPTURE_REVIEW, output = loop only. When mic pass-through needed, mix mic + loop.

3. Add volume control:
   - Master volume from encoder (in appropriate stage)
   - Apply gain scaling before I2S write
   - Clamp to prevent clipping: `max(-32767, min(32767, sample))`

4. Create FreeRTOS audio task:
   - Pin to Core 1, priority 5
   - 1ms period (or driven by I2S DMA callback)
   - Calls `AudioEngine_process()` each tick
   - Uses `AudioHAL_readMic()` and `AudioHAL_writeSpeaker()`

**Critical detail — I2S stereo handling:**
The I2S bus is configured for stereo. `AudioHAL_readMic()` returns interleaved L/R samples. For mono:
```cpp
// Extract mono from stereo input
for (int i = 0; i < frameCount; i++) {
    mono[i] = stereoInput[i * 2];  // Take left channel only
}
// Duplicate mono to stereo output
for (int i = 0; i < frameCount; i++) {
    stereoOutput[i * 2] = monoOutput[i];
    stereoOutput[i * 2 + 1] = monoOutput[i];
}
```

**Files to modify:**
- `firmware/src/core/retroactive_buffer.h` / `.cpp`
- `firmware/src/core/audio_engine.h` / `.cpp`
- `firmware/src/config.h` (buffer sizing constants)
- `firmware/src/main.cpp` (FreeRTOS audio task)

---

### Phase 4: BPM Clock
**Goal:** Port the web prototype's BPM clock for synchronized chop timing.
**Depends on:** Phase 3 (needs audio running for time reference).

**Context from web prototype (clock.js):**
- 120 BPM default
- Emits events: `sixteenth`, `eighth`, `beat`, `bar`
- Lookahead scheduler: polls every 25ms, schedules 100ms ahead
- Used by chop engine for slice timing and chord quantization

**Tasks:**
1. Create `firmware/src/core/bpm_clock.h`:
   ```cpp
   typedef void (*ClockCallback)(uint32_t bar, uint8_t beat, uint8_t sub);

   void BPMClock_init(uint16_t bpm);
   void BPMClock_start();
   void BPMClock_stop();
   void BPMClock_setBPM(uint16_t bpm);
   void BPMClock_tick();  // Call once per sample in audio callback
   void BPMClock_onSixteenth(ClockCallback cb);
   void BPMClock_onBar(ClockCallback cb);
   uint32_t BPMClock_getCurrentBar();
   uint8_t BPMClock_getCurrentBeat();
   float BPMClock_barDurationSec();

   // Tap-tempo
   void BPMClock_tap();          // Call on each encoder push tap
   bool BPMClock_isTapActive();  // True during tap recording
   ```

2. Create `firmware/src/core/bpm_clock.cpp`:
   - Sample-counting approach (more accurate than timer-based):
     - `samplesPerSixteenth = SAMPLE_RATE * 60 / (bpm * 4)`
     - At 120 BPM, 16kHz: `samplesPerSixteenth = 16000 * 60 / 480 = 2000 samples`
   - `BPMClock_tick()` incremented once per sample in audio callback
   - When sample counter reaches `samplesPerSixteenth`, fire callback, reset counter, advance sub
   - sub 0-15 maps to 16th notes in a bar; sub 0,4,8,12 = beats; sub 0 = bar downbeat

3. **Tap-tempo** (encoder push):
   - When encoder push held and released, start recording tap intervals
   - Accumulate last 4-8 tap intervals, compute average BPM
   - Ignore intervals > 2 seconds (reset on gap)
   - Tap-tempo active only in CHOP/RESONATE stages
   - Visual feedback: LED blink or display update on each tap

4. **Encoder BPM adjust** (when not in tap-tempo mode):
   - CW = +1 BPM, CCW = -1 BPM
   - Long turn = ±5 BPM per click
   - Range: 60-180 BPM

3. Wire into audio engine:
   - Call `BPMClock_tick()` once per sample inside `AudioEngine_processBuffer()`
   - Clock callbacks trigger chop slice playback and chord quantization

**Key difference from web:** The web uses `setTimeout` with lookahead. On ESP32, we count samples directly in the audio ISR — zero jitter, sample-accurate timing.

**Files to create:**
- `firmware/src/core/bpm_clock.h`
- `firmware/src/core/bpm_clock.cpp`

---

### Phase 5: Transient Detection + Chop Engine
**Goal:** Port slice boundary detection and Euclidean rhythm playback from the web prototype.
**Depends on:** Phase 3 (needs captured loop buffer), Phase 4 (needs BPM clock).

**Context from web prototype:**
- `transient-detector.js`: Spectral flux + RMS volume gate → onset times → slice boundaries
- `chop-engine.js`: Bjorklund Euclidean pattern, 3 playback types (normal/reverse/ratchet)

**Tasks:**

#### 5a: Simplified Transient Detection
The web prototype uses Meyda for spectral analysis (1024-sample FFT). On ESP32 at 16kHz without PSRAM, we simplify:

1. Create `firmware/src/audio/transient_detector.h` / `.cpp`:
   - **Amplitude-based detection** (skip FFT for now — saves ~16KB RAM and significant CPU):
     - Compute RMS energy per frame (256 samples = 16ms)
     - Smoothed envelope follower: `env = max(env * 0.95, rms)`
     - Onset when `rms > env * threshold` AND `rms > noiseFloor`
     - Minimum gap between onsets: 50ms (800 samples at 16kHz)
   - Output: array of onset sample positions (max 32)
   - Grid fallback: if < 2 onsets detected, divide buffer evenly at BPM grid

2. Parameters:
   ```cpp
   #define ONSET_THRESHOLD     1.5f   // RMS must exceed envelope by 1.5×
   #define MIN_ONSET_GAP_MS    50     // Minimum 50ms between onsets
   #define MAX_SLICES          32
   #define MIN_SLICES          2
   ```

#### 5b: Chop Engine (Euclidean Rhythmic Slicer)

1. Create `firmware/src/audio/chop_engine.h` / `.cpp`:
   - **Bjorklund algorithm** (port directly from chop-engine.js):
     ```cpp
     void bjorklund(uint8_t pulses, uint8_t steps, bool* pattern);
     ```
   - **State:**
     ```cpp
     struct ChopState {
         int16_t* sliceStarts;    // Sample offsets into loop buffer
         int16_t* sliceLengths;   // Length of each slice in samples
         uint8_t numSlices;
         bool pattern[16];        // Euclidean step pattern
         uint8_t playbackType[16]; // 0=normal, 1=reverse, 2=ratchet
         float density;           // 0-1, maps to 2-16 pulses
         uint8_t currentStep;
         bool active;
     };
     ```
   - **On clock sixteenth callback:**
     - If `pattern[currentStep]` is true AND `active`:
       - Select slice: `sliceIdx = currentStep % numSlices`
       - Copy slice samples to output buffer (normal, reversed, or ratcheted)
     - Advance `currentStep = (currentStep + 1) % 16`
   - **Density control:**
     - `pulses = 2 + (int)(density^0.7 * 14)` (same nonlinear curve as web)
     - Recalculate Bjorklund pattern when density changes
   - **Playback types** (from chop-engine.js):
     - Normal (77%): play slice forward with 2ms attack ramp
     - Reverse (15%): play slice backward
     - Ratchet (8%): 2-3 quick repetitions at 32nd-note spacing
   - **Randomize:** joystick center re-rolls playback types for all active steps

2. Wire into audio engine:
   - ChopEngine receives reference to captured loop buffer
   - On sixteenth callback, writes slice audio into a chop output buffer
   - Audio engine crossfades between raw loop and chop output (50ms fade)

**Files to create:**
- `firmware/src/audio/transient_detector.h` / `.cpp`
- `firmware/src/audio/chop_engine.h` / `.cpp`

**Files to modify:**
- `firmware/src/audio/dsp/chopper.h` / `.cpp` — replace stub with real engine call
- `firmware/src/core/audio_engine.cpp` — integrate chop into signal chain

---

### Phase 6: Faust Resonator Compilation + Integration
**Goal:** Compile `resonatorTest2.dsp` to C++ and wire it into the audio engine.
**Depends on:** Phase 3 (needs audio input from loop/chop), Phase 4 (for chord timing).

**Context:**
The Faust DSP file at `firmware/faust/resonatorTest2.dsp` is the same resonator used in the web prototype (compiled to WASM there). It has 30+20 resonator bands, Zita reverb, and 16 chord types. This is the most CPU-intensive component.

**Tasks:**

#### 6a: Compile Faust to C++
1. Install Faust compiler if not present: `brew install faust`
2. Modify `firmware/faust/resonatorTest2.dsp`:
   - Remove `dm.zita_rev1` from the main process — replace with Freeverb (lighter, ~10× less memory)
   - Reduce `nHarmonics` from 6 to 3 (15 bands instead of 30) to save CPU + memory
   - Disable Engine B (spectral mask, 20 bands) — keep only Engine A (chord resonator)
   - Revised DSP: 5 voices × 3 harmonics = 15 mode filters + Freeverb, no spectral mask
3. Compile:
   ```bash
   faust -cn MorphingResonator -i -a minimal.cpp firmware/faust/resonatorTest2.dsp -o firmware/src/audio/dsp/faust_resonator.cpp
   ```
4. The generated C++ class has: `init(sampleRate)`, `compute(count, inputs, outputs)`, `setParamValue(path, value)`

#### 6b: Integration Wrapper
Same as v1 plan — int16_t ↔ float conversion, simple API (setChord, setMix, setTimbre, etc.).

#### 6c: Memory Budget Check
- Revised Faust DSP: 15 mode filters + Freeverb. Estimate ~80-120 KB of state. Use PSRAM.
- Measure with `ESP.getFreePsram()` before and after resonator init.

#### 6d: CPU Budget Check
- 15 mode filters + Freeverb at 16kHz on 240MHz ESP32-S3. Target: <50% Core 1.
- Profile with `micros()` around `Resonator_process()`.
- If too heavy: further reduce to 2 harmonics per voice (10 bands total).

**Files to create:**
- `firmware/src/audio/dsp/faust_resonator.cpp` (generated)
- `firmware/src/audio/dsp/resonator_wrapper.h` / `.cpp`

**Files to modify:**
- `firmware/src/audio/dsp/resonator.h` / `.cpp` — replace stub
- `firmware/src/core/audio_engine.cpp` — route loop/chop into resonator

---

### Phase 7: Circular UI — LVGL Waveform + Stage Display
**Goal:** Replace the basic LVGL status label with the circular waveform display and stage-appropriate visualizations.
**Depends on:** Phase 2 (stage info), Phase 3 (audio data for waveform), Phase 5 (chop state for visualization).

**Context from web prototype:**
The web app renders to a 466×466 canvas with circular clipping. Key visuals:
- IDLE: pulsing waveform hint + stage indicator dots
- CAPTURE_REVIEW: circular waveform with window highlight, pitch indicator
- CHOP: circular waveform + slice markers + density ring + 16-step grid
- RESONATE: chord selection ring (7-8 chords displayed around circle)

**Tasks:**

#### 7a: LVGL Canvas Setup
1. Create `firmware/src/ui/circular_display.h` / `.cpp`:
   - With PSRAM available, full 466×466 canvas buffer (~434 KB) fits in PSRAM
   - Use LVGL `lv_canvas` with PSRAM-allocated buffer for radial waveform drawing
   - Alternative: Use Arduino_GFX directly for waveform layer + LVGL for text/widgets (avoids LVGL canvas complexity)
   - Circular clip via `lv_draw_mask` — draw circle, add arc segments, clip content

#### 7b: Waveform Visualization (Simplified for ESP32)
1. Create `firmware/src/ui/waveform_view.h` / `.cpp`:
   - Downsample audio buffer to ~120 points (enough for circular display)
   - Draw as radial lines from center outward, scaled by amplitude
   - Color: amber for CHOP, green for RESONATE, white for CAPTURE_REVIEW
   - Use Arduino_GFX `drawLine()` — faster than full canvas bitmap

2. Playhead: single bright line sweeping around circle at loop rate

#### 7c: Stage Indicator
1. 4 dots at compass points showing current stage (from visualizer.js):
   - IDLE=dim, CAPTURE=orange, CHOP=amber, RESONATE=green
   - Current stage dot is bright, others dim

#### 7d: Center Info Display
1. LVGL labels centered on screen:
   - IDLE: "IDLE" + listening indicator
   - CAPTURE_REVIEW: bar count + pitch offset
   - CHOP: density percentage + "DEN"
   - RESONATE: chord name + root note

#### 7e: Chop Visualization
1. Slice boundary markers as colored arcs around waveform
2. 16-step grid in center (small dots, colored by playback type)
3. Density ring: outer arc showing fill level

#### 7f: Chord Selection Display (RESONATE)
1. Ring of 7 chord labels around the circle edge
2. Current chord highlighted with glow
3. Root note displayed in center

**Performance notes:**
- Target 30fps for UI updates (33ms per frame)
- Only redraw changed regions (LVGL dirty area system)
- Waveform redraw only when audio data changes or playhead moves
- Use LVGL timer for periodic refresh (not tight loop)

**Files to create:**
- `firmware/src/ui/circular_display.h` / `.cpp`
- `firmware/src/ui/waveform_view.h` / `.cpp`
- `firmware/src/ui/stage_display.h` / `.cpp`
- `firmware/src/ui/chop_view.h` / `.cpp`
- `firmware/src/ui/chord_view.h` / `.cpp`

**Files to modify:**
- `firmware/src/core/ui_manager.cpp` — replace placeholder with real UI system

---

### Phase 8: Capture Review — Window Selection + Pitch Shift
**Goal:** Port the capture review stage where user selects loop window length and adjusts pitch.
**Depends on:** Phase 3 (captured buffer), Phase 7 (display), Phase 1 (encoder for pitch).

**Context from web prototype (capture-intelligence.js):**
- After capture, auto-selects 2 or 4 bar window based on buffer length + BPM
- User can cycle through 1/2/4/8 bar windows (joystick up/down)
- User can nudge window position (joystick, snaps to bar boundaries)
- Encoder adjusts pitch ±12 semitones
- Display shows circular waveform with highlighted window region

**Tasks:**
1. Create `firmware/src/audio/capture_review.h` / `.cpp`:
   - `setLoopBuffer(int16_t* buf, uint32_t length)`
   - `setWindowBars(uint8_t bars)` — 1, 2, 4, 8
   - `nudgeWindow(int8_t barDelta)` — shift window position, snap to bar boundary
   - `confirm()` — returns pointer + length of selected window region
   - Bar-aligned: `barSamples = SAMPLE_RATE * 60 * 4 / bpm`

2. Pitch shift (simplified for ESP32):
   - **Option A (simple):** Playback rate change — adjust loop read pointer increment
     - `readIncrement = pow(2.0, semitones / 12.0)`
     - Faster/slower playback = pitch up/down (changes tempo too — acceptable for lo-fi)
   - **Option B (better, more CPU):** Linear interpolation between samples for fractional read positions
   - Start with Option A; it matches the lo-fi aesthetic

3. Encoder mapping in CAPTURE_REVIEW stage:
   - CW = pitch up 1 semitone (max +12)
   - CCW = pitch down 1 semitone (min -12)
   - Display shows current pitch offset

**Files to create:**
- `firmware/src/audio/capture_review.h` / `.cpp`

**Files to modify:**
- `firmware/src/core/stage_manager.cpp` — CAPTURE_REVIEW stage logic
- `firmware/src/core/audio_engine.cpp` — pitch-shifted playback

---

### Phase 9: Full Signal Chain Integration
**Goal:** Wire all audio components together into the complete signal path.
**Depends on:** Phases 3, 4, 5, 6, 8 (all audio components).

**Signal chain (matching web prototype):**
```
Mic → RetroactiveBuffer (always, until capture)
         ↓ (on capture)
    CapturedLoop (windowed)
         ↓
    PitchShift (if in CAPTURE_REVIEW, encoder)
         ↓
    Loop Playback
         ↓
    ┌──────────────────────────┐
    │  Chop OFF: raw loop      │
    │  Chop ON:  ChopEngine    │ ← crossfade (50ms)
    └──────────────────────────┘
         ↓
    ┌──────────────────────────┐
    │  Resonator OFF: dry pass │
    │  Resonator ON:  Faust    │ ← wet/dry mix (encoder)
    └──────────────────────────┘
         ↓
    Volume scaling
         ↓
    Mono → Stereo duplication
         ↓
    I2S DMA Write
```

**Tasks:**
1. Refactor `AudioEngine_processBuffer()` to implement the full chain above
2. Crossfade implementation:
   ```cpp
   // 50ms fade = 800 samples at 16kHz
   #define CROSSFADE_SAMPLES 800
   float fadeGain = (float)fadeCounter / CROSSFADE_SAMPLES;
   output[i] = (int16_t)(loop[i] * (1.0f - fadeGain) + chop[i] * fadeGain);
   ```
3. Wet/dry mix (equal-power crossfade, from resonatorTest2.dsp):
   ```cpp
   float dryGain = cosf(mix * M_PI / 2.0f);
   float wetGain = sinf(mix * M_PI / 2.0f);
   output[i] = (int16_t)(dry[i] * dryGain + wet[i] * wetGain);
   ```
4. Ensure audio task never blocks — all DSP must complete within one frame period

**Files to modify:**
- `firmware/src/core/audio_engine.cpp` — full signal chain

---

### Phase 10: SD Card Recording (Future)
**Goal:** Save captured loops to SD card as WAV files.
**Depends on:** Phase 9 (complete audio chain), storage HAL (already implemented).

**Tasks:**
1. WAV file writer (16-bit PCM, mono @ 16kHz initially)
2. On capture confirm, write loop buffer to SD card with timestamp filename
3. Future: write stereo using both ES7210 mic channels
4. File browser UI for loading saved loops

**Deferred — not needed for MVP.**

---

## Dependency Graph

```
Phase 0 (Volume Fix + Memory Audit)
   ↓
Phase 1 (MCP23017 Input Driver)
   ↓
Phase 2 (State Machine) ←──────────────┐
   ↓                                    │
Phase 3 (Audio Pipeline) ──────────┐    │
   ↓                               │    │
Phase 4 (BPM Clock)               │    │
   ↓                               │    │
Phase 5 (Transient + Chop) ←──────┘    │
   ↓                                    │
Phase 6 (Faust Resonator)              │
   ↓                                    │
Phase 7 (Circular UI) ←────────────────┘
   ↓
Phase 8 (Capture Review)
   ↓
Phase 9 (Full Integration)
   ↓
Phase 10 (SD Card — future)
```

**Parallel work possible:**
- Phase 1 (input) and Phase 3 (audio) can develop in parallel after Phase 0
- Phase 4 (clock) can develop alongside Phase 3
- Phase 7 (UI) can start after Phase 2, developing visual components while audio phases progress

## Resolved Constraints

All open questions resolved by the hardware team:

- **PSRAM:** Board is ESP32-S3R8 (8MB OPI PSRAM) with dedicated Octal SPI bus. Enable with `board_build.psram = opi`. Verify in Phase 0.
- **MCP23017 wiring:** Fully verified — see the "Verified pin mapping" table above.
- **Headphone volume:** Try ES8311 software attenuation first (Phase 0), hardware fix only if needed.
- **BPM/tap-tempo:** Tap-tempo on encoder push is required (Phase 4).
- **Resonator DSP:** Remove Zita reverb (use Freeverb). Reduce to 15 bands (5 voices × 3 harmonics), disable Engine B spectral mask (Phase 6).

---

## Implementation Audit (Phases 0–5) — 2026-04-28

An agent attempted to implement Phases 0–5 in a single pass. The following documents what was implemented, what's broken, and what needs to be fixed before each phase can be considered complete.

---

### Phase 0: Volume Fix + PSRAM Enable + Memory Audit

#### PSRAM Enable — PASS
- `platformio.ini` line 42: `board_build.psram = opi` ✓
- Build flags define `-DBOARD_HAS_PSRAM` and `-DPSRAM_FIXED` ✓
- `config.h` line 28: `#define PSRAM_AVAILABLE 1` ✓

#### Memory Audit — PASS (minor gaps)
- `main.cpp` logs `ESP.getFreeHeap()` and `ESP.getFreePsram()` at 6+ init stages ✓
- **Minor**: Lines for "After stage manager" and "After BPM clock" only log heap, not PSRAM

#### Headphone Volume Attenuation — CRITICAL BUGS

| Issue | Severity | Details |
|-------|----------|---------|
| Wrong register for DAC volume | CRITICAL | `es8311_write_reg(0x22, 0x40)` — register 0x22 is ADC HPF config, not DAC output volume |
| Missing PA gain register | CRITICAL | Register 0x32 (DAC output amplifier gain) is never written |
| `AudioHAL_setVolume()` wrong register | HIGH | Writes to register 0x14 (System Control / bias), not a volume register |
| No headphone-specific attenuation | HIGH | NS4150B PA routes directly to headphone jack via PJ307 — needs software gain reduction before I2S write |

**Fix required**: Use correct ES8311 registers. Register 0x32 controls PA gain. Add a software volume multiplier (e.g., `sample = sample * volume / 256`) before the I2S write as a reliable fallback.

---

### Phase 1: MCP23017 Input Driver + Encoder Decoding

#### Correct
- `mcp_input.h`: Event enum, InputId enum, InputMsg struct, API declarations all correct
- `MCPInput_init()`: Wire2 on GPIO 16/17, MCP23017 at 0x27, pull-ups, INTA on GPIO 18 — all correct
- `input_task.cpp`: 10ms poll loop on Core 0, routes events to `StageManager_handleInput()` ✓
- Timing constants correct: DEBOUNCE_MS=50, DOUBLECLICK_MS=300, LONGPRESS_MS=500

#### Critical Bugs

| Bug | Severity | Location | Details |
|-----|----------|----------|---------|
| Button press/release logic inverted | CRITICAL | mcp_input.cpp ~L135-182 | `if (pressed)` block handles release logic and vice versa. All button events fire on wrong edge |
| Double-click detection broken | CRITICAL | mcp_input.cpp ~L150-160 | Checks queue for previous press event BEFORE queuing current event — previous press is never in queue |
| Joystick fires continuously | CRITICAL | mcp_input.cpp ~L274-285 | Events push every 10ms poll while held, not on edge transitions. Stage navigation will be chaotic |
| Encoder delta double-counted | HIGH | input_task.cpp ~L28-34 | Counts EVT_ENCODER_CW/CCW events AND calls `MCPInput_getEncoderDelta()` — each click counted twice |
| Shift state incomplete | HIGH | mcp_input.cpp ~L272 | Only tracks held state (`shiftHeld = btn2`). Plan requires tap-toggle and double-tap-latch |
| No FreeRTOS queue | MEDIUM | mcp_input.cpp ~L32-34 | Uses plain circular array instead of `xQueueSend()`/`xQueueReceive()` — not thread-safe |
| EVT_BUTTON_RELEASE never fires | LOW | mcp_input.cpp | Defined in enum but no code path emits it |

**Fix required**: Rewrite `processButton()` with correct press/release polarity. Add edge detection to joystick. Remove double encoder accumulation. Implement shift tap/latch state machine.

---

### Phase 2: State Machine + Event Dispatch

#### Correct
- Stage enum: STAGE_IDLE, STAGE_CAPTURE_REVIEW, STAGE_CHOP, STAGE_RESONATE ✓
- All state transitions match spec (IDLE→CAPTURE_REVIEW→CHOP↔RESONATE) ✓
- Shift+Capture reset to IDLE from any stage ✓
- `enterStage()` calls audio engine functions for capture/playback ✓
- Integration in main.cpp: `StageManager_init()` + `StageManager_update()` in loop ✓

#### Issues

| Issue | Severity | Details |
|-------|----------|---------|
| All 9 parameter handlers are TODO stubs | HIGH | Encoder→pitch, encoder→density, encoder→wet/dry, joy→bar length, joy→chord select — all empty |
| No exit handlers | LOW | Only `enterStage()` exists, no cleanup on exit |
| `StageManager_update()` is empty | LOW | Called every tick but does nothing |
| Audio engine functions called but don't exist | HIGH | `AudioEngine_setChopEnabled()`, `AudioEngine_setResonatorEnabled()` referenced but undefined |

**Fix required**: Parameter handlers can't be wired until Phase 3-6 audio engine APIs exist. The skeleton is correct — this phase is blocked on downstream work.

---

### Phase 3: Audio Pipeline — Retroactive Capture + Loop Playback

#### Critical Issues

| Issue | Severity | Details |
|-------|----------|---------|
| Retroactive buffer allocated STEREO | CRITICAL | `bufferSize = SAMPLE_RATE * bufferLengthSeconds * 2` — plan specifies mono. Wastes 50% of PSRAM budget |
| No FreeRTOS audio task on Core 1 | CRITICAL | `AudioEngine_process()` is defined but never called. No `xTaskCreatePinnedToCore()` for audio. The entire audio path is dead |
| Stereo/mono confusion throughout | HIGH | Buffer writes stereo interleaved (`i * 2`, `i * 2 + 1`) but plan is mono internally |
| No volume control stage | MEDIUM | Only ad-hoc soft clipping at ±16383, no actual gain control |
| Incomplete signal chain | MEDIUM | Resonator not in audio path. Flow stops at chopper output |

**Fix required**: Remove stereo multiplier from buffer allocation. Create audio task pinned to Core 1 with high priority. Fix all buffer read/write to mono. Add volume scaling stage before I2S write.

---

### Phase 4: BPM Clock

#### Correct
- Sample-counting formula: `samplesPerSub = SAMPLE_RATE * 60 / (bpm * 4)` ✓
- Sixteenth/bar callbacks with proper position tracking (bar/beat/sub) ✓
- Tap-tempo: 8-tap history, 2-second timeout, 3-tap minimum, average computation ✓
- BPM range clamped 60–180 ✓
- `BPMClock_init()`, start/stop, position getters all implemented ✓

#### Issues

| Issue | Severity | Details |
|-------|----------|---------|
| BPM clock never ticked | CRITICAL | `BPMClock_tick()` must be called once per sample in audio callback — but audio task doesn't exist (Phase 3 blocker) |
| Tap-tempo never called | HIGH | `BPMClock_tap()` exists but no input path calls it |
| Encoder BPM adjust not wired | HIGH | `BPMClock_setBPM()` exists but Stage Manager has TODO stubs for encoder handling |
| Sixteenth callback not wired to chopper | HIGH | Callback registered but no `ChopEngine_onSixteenth()` or equivalent triggered |

**Fix required**: Phase 4 is logically complete but completely disconnected. Once the Core 1 audio task exists (Phase 3 fix), wire `BPMClock_tick()` into it. Wire tap-tempo to encoder push in CHOP/RESONATE stages. Connect sixteenth callback to chop engine trigger.

---

### Phase 5: Transient Detection + Chop Engine

#### Phase 5a: Transient Detection — 95% Complete

| Component | Status |
|-----------|--------|
| RMS energy per 256-sample frame | ✓ |
| Envelope follower (env × 0.95) | ✓ |
| Onset detection (rms > env × threshold AND > noise floor) | ✓ |
| 50ms minimum gap (800 samples) | ✓ |
| Max 32 slices | ✓ |
| Grid fallback if < 2 onsets | ✓ |

**One issue**: RMS frame buffer capped at 256 entries (`float rmsFrames[256]`). At 16kHz with 256-sample frames, this only covers ~4 seconds of audio. Captures longer than 4s will have onsets detected only in the first 4 seconds. **Fix**: Use dynamic allocation or increase cap to 1024 (covers ~16s).

#### Phase 5b: Chop Engine — 45% Complete

| Component | Status | Details |
|-----------|--------|---------|
| Bjorklund algorithm | ✓ | Correct Euclidean pattern generation |
| Density formula (2 + density^0.7 × 14) | ✓ | Matches spec exactly |
| Playback type probabilities (77/15/8) | ✓ | Correct distribution |
| Randomize on joystick center | ✓ | `ChopEngine_randomize()` re-rolls types |
| **Actual slice playback** | MISSING | `sliceIdx` computed but never used to read audio data |
| **Playback type execution** | MISSING | `ptype` fetched but ignored — no reverse/ratchet logic |
| **2ms attack ramp** | MISSING | No fade-in applied to slice starts |
| **Crossfade muting** | MISSING | `mixCrossfade()` helper defined but never called |

**Fix required**: Implement the actual slice playback engine — read from captured loop buffer at slice boundaries, apply attack ramp, implement reverse (read backwards) and ratchet (2-3× rapid repeat at 32nd-note spacing) playback modes.

---

### Cross-Cutting Issues

| Issue | Affects | Details |
|-------|---------|---------|
| No Core 1 audio task | Phase 3, 4, 5 | The entire real-time audio path is dead code. Nothing calls `AudioEngine_process()` or `BPMClock_tick()` |
| Stereo/mono confusion | Phase 3 | Buffer allocation, read, and write inconsistently mix stereo and mono assumptions |
| Disconnected components | Phase 2, 4, 5 | Stage manager, BPM clock, and chop engine are all implemented in isolation with no wiring between them |
| No integration testing possible | All | Without the audio task, no end-to-end signal flow can be verified on hardware |

---

### Recommended Fix Order

1. **Phase 0 audio registers** — Fix ES8311 register addresses (blocks headphone testing)
2. **Phase 3 audio task** — Create Core 1 FreeRTOS task that calls `AudioEngine_process()` (unblocks everything)
3. **Phase 3 mono buffer** — Remove stereo multiplier, fix all read/write paths
4. **Phase 1 button logic** — Fix inverted press/release, joystick edge detection, encoder double-count
5. **Phase 4 wiring** — Call `BPMClock_tick()` from audio task, wire tap-tempo to input
6. **Phase 5b slice playback** — Implement actual audio slice reading with attack ramp and playback types
7. **Phase 2 parameter handlers** — Wire encoder/joystick to audio engine APIs (last, depends on all above)
