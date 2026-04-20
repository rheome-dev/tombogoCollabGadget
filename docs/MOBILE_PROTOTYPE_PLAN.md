# Plan: Musical Framework Mobile Prototype

## Context

The Tombogo x Rheome Collab Gadget has a working web-based Morphing Resonator but no musical framework (looping, clock, chopping). The goal is to build a **hardware-faithful mobile prototype** that simulates the exact physical device UI — a 1.75" round AMOLED (466×466), 5-way joystick, rotary encoder, 3 buttons, gyroscope — so we can validate the interaction paradigm before porting to ESP32.

**This is NOT a generic web app. It's a 1:1 simulation of the physical device.**

---

## Device Interaction Model (from interview)

### Signal Pipeline (One Unified Flow)
```
[CAPTURE] → [CHOP] → [RESONATE]
```
These are NOT separate modes — they're stages in one pipeline. The UI auto-advances through stages as the user works (capture → window adjust → auto-chop → resonate). The user can jump back to any stage at any time via joystick left/right.

### Physical Controls → Web Emulation

| Physical Control | Web Emulation | Default Function | Shift+ Function |
|-----------------|---------------|------------------|-----------------|
| **Capture button** | Dedicated on-screen button | Capture (always). 2nd press = layer | Shift+Capture = clear layer |
| **Shift button** | Dedicated on-screen button (hold) | Modifier for all other controls | — |
| **Button 3** | On-screen button | TBD / context-dependent (discover during prototyping) | — |
| **5-way joystick** | Touch joystick widget (4 directions + center tap) | Left/Right = navigate pipeline stages. Up/Down = context in stage. Center = primary action | Shift+joystick = extended nav |
| **Rotary encoder** | Touch ring/drag gesture | Volume (turn). Mute (push) | Shift+turn = context param (timbre/density/BPM per stage) |
| **Touchscreen (round)** | 466×466 round viewport (exact size) | Chord selection, waveform interaction, X-Y pad (stage-dependent) | Shift+touch = extended gestures |
| **Gyroscope/IMU** | DeviceMotion API | Inactive (prevents accidental changes) | Shift+tilt = global expression macro (reverb, filter, decay simultaneously) |

### Display Rules (Dead-Front Aesthetic)
- **Background: always pure black** (OLED pixels off = invisible through dead-front overlay)
- **No text, no menus** — symbols/icons only, all glowing on black
- **Stage indicators:** Small colored glowing icons at display edge. Current stage = highlighted/animated
- **Color language per stage:** Each stage has a distinct icon color (not background)
- **Everything at surface level or 1 level deep maximum**

### Chord UI
- **Start with 7-8 diatonic chords as larger touch targets** filling the round display
- Extended chords accessible via shift+touch or encoder scroll
- **Future:** Dynamic contextual mode where center = current chord, surrounding circles = musically likely next chords based on voice-leading/harmonic direction (up = brighter/resolution, down = darker/tension)
- All chord changes quantized to 8th notes by default (toggleable)

### Capture Window Adjustment (Ableton-style intelligent capture)
- System auto-detects bar-aligned boundaries when capture is pressed
- **Encoder turn = shift window position** (nudge earlier/later in time)
- **Touch on waveform = adjust window length** (select 1/2/4/8 bars)
- Musical intelligence: system infers intent based on BPM and recent activity
- Double/triple click patterns for secondary functions

---

## Phase 1: Device Simulation Shell + Audio Foundation

**Goal:** Build the hardware-faithful UI shell and get mic → circular buffer → capture working.

### 1a. Create `web/device-shell.js` — Round display simulator
- Render a 466×466 circular viewport (exact device size at 1:1 on phone screens)
- Pure black background, round clip mask
- Everything outside the circle is black/hidden
- Scale for different phone screens but maintain aspect ratio

### 1b. Create `web/controls-ui.js` — Physical control emulation
- **Joystick widget:** Small 5-way touch pad below the round display (or overlay)
  - Touch zones for up/down/left/right/center
  - Visual: 5 glowing dots in a cross pattern
- **Encoder widget:** Circular drag gesture area
  - Drag clockwise/counterclockwise = turn
  - Tap center = push
  - Visual: ring indicator showing value
- **3 Buttons:** Three distinct touch zones
  - Capture (always labeled with capture icon), Shift (hold-to-activate), Button 3
  - Visual: glowing symbols, shift glows while held
- All positioned to approximate physical device ergonomics

### 1c. Create `web/circular-buffer-worklet.js` — Always-on mic recording
- AudioWorklet processor: continuously writes mic input to ring buffer
- 30 seconds mono @ 44100Hz = ~5.3MB Float32
- On "capture" message from main thread, copies buffer contents back via postMessage (transferable)
- Mirrors firmware `RetroactiveBuffer` API

### 1d. Create `web/clock.js` — Sample-accurate BPM clock
- AudioWorklet-based (no setTimeout drift)
- Default 120 BPM, configurable
- Beat/subdivision events (8th notes, 16th notes)
- `getNextBeatTime()` for chord quantization
- `getCurrentBar()` for capture intelligence

### 1e. Create `web/app.js` — Main application controller
- Pipeline state machine: IDLE → CAPTURE_REVIEW → CHOP → RESONATE
- Auto-advance logic: capture confirmed → auto-detect transients → show chop stage
- Routes all control inputs to the correct handler based on current stage
- AudioContext initialization on first user gesture (iOS requirement)
- Wire capture button → circular buffer capture → window review

### 1f. Create `web/capture-intelligence.js` — Smart capture window
- When capture pressed: grab 30s buffer, analyze for musical boundaries
- Use BPM clock to find bar-aligned start/end points
- Default to nearest 2 or 4 bar window
- Encoder adjusts position, touch adjusts length (1/2/4/8 bars)
- Visual: waveform with highlighted capture window on round display

### 1g. Restructure `web/index.html` + `web/style.css`
- Remove all existing slider-based UI
- Single round viewport with control widgets around it
- Mobile-first, full-screen, no scroll
- All black except glowing elements
- Script tags for new module structure

**Files:** `web/device-shell.js`, `web/controls-ui.js`, `web/circular-buffer-worklet.js`, `web/clock.js`, `web/app.js`, `web/capture-intelligence.js`, `web/index.html`, `web/style.css` (all new or major rewrite)

---

## Phase 2: Transient Detection + Chop Engine

**Goal:** Captured audio is auto-analyzed and chopped into rhythmic patterns.

### 2a. Create `web/transient-detector.js`
- Offline spectral flux onset detection (runs once after capture, not real-time)
- Algorithm: STFT (512 hop) → positive spectral flux → adaptive threshold → peak-pick
- Tuned for ambient/textural (lower sensitivity, min slice ~100ms)
- **Fallback:** If too few onsets, equal-divide at BPM grid (guarantees rhythm from any input)

### 2b. Create `web/chop-engine.js` — Rhythmic slice sequencer
- Takes captured audio + onset positions + rhythm parameters + clock reference
- Pattern generation:
  - Quantize onsets to rhythmic grid
  - **Density** (0-1): sparse → dense. Strong beats (1, 3) preferred at low density
  - **Variation** (0-1): repetitive → mutating each cycle
- Schedules AudioBufferSource nodes via clock timing
- 5ms crossfade at slice boundaries
- Textural/ambient focus (not drum-beat focused like XLN Life)

### 2c. Create `web/visualizer.js` — Round display visualizations
- All visuals rendered inside the 466×466 circle, black background, glowing elements
- **Capture stage:** Live waveform ripple (mic level). After capture: waveform with window bracket
- **Chop stage:** Waveform with glowing slice markers. Active slice highlights on beat
- **Resonate stage:** Harmonic ring visualization (frequency bands glowing with resonance)
- Stage indicator icons at display edge (colored glowing symbols)

### 2d. Chop stage controls mapping
- **Shift+encoder turn:** Adjusts density (the primary chop param in this stage)
- **Joystick up/down:** Context action in chop stage (variation, or cycle through pattern presets)
- **Touch screen:** X-Y pad area for density (X) / variation (Y) — alternative to encoder
- **Joystick right:** Advance to resonate stage
- **Joystick left:** Go back to capture/window adjustment

**Files:** `web/transient-detector.js`, `web/chop-engine.js`, `web/visualizer.js` (all new)

---

## Phase 3: Resonator Fixes + Pipeline Integration

**Goal:** Fix the resonator DSP issues and complete the Capture → Chop → Resonate chain.

### 3a. Fix timbre slider in `web/dsp.js`
**Problem:** Linear frequency blending between harmonic modes creates dead spots at non-integer values.
**Fix:**
- Interpolate in **log space**: `freq = Math.exp(Math.log(harmF)*w0 + Math.log(inharmF)*w1 + Math.log(fixedF)*w2)`
- Equal-power (cosine) crossfade weights instead of linear triangle
- Per-harmonic gain rolloff: `1/sqrt(h+1)`

### 3b. Fix distortion in `web/dsp.js`
**Root cause:** JS has `* 40` gain multiplier on engine outputs (lines 265-266) that doesn't exist in Faust source. **40x gain difference.**
**Fix:**
- Remove `* 40` from engine gain calculations
- Remove `* 4` per-filter gain (line 97)
- Reduce Q: `decay * 15` instead of `* 30`
- Adjust wet trim default upward to compensate

### 3c. Route chop engine → resonator
- ChopEngine output connects to MorphingResonator inputNode
- Chopped audio becomes primary resonator exciter
- When no chop is active, mic/pluck still available

### 3d. Chord UI on round display
- 7-8 diatonic chord circles filling the round display (resonate stage view)
- Current chord = center/highlighted, others arranged around it
- Touch to select, quantized to next 8th note via clock
- Shift+touch = access extended chords (9ths, 11ths, sus4)

### 3e. Chord quantization
- `setParam('chordIdx')` schedules change at next 8th note boundary
- Toggle via shift+joystick center (or discoverable during prototyping)

### 3f. Resonate stage controls mapping
- **Touchscreen:** Chord selection (primary interaction in this stage)
- **Shift+encoder turn:** Timbre morph (the primary resonator param)
- **Shift+tilt (gyro):** Global expression macro (reverb + filter + decay simultaneously)
- **Joystick left:** Go back to chop stage
- **Encoder (unmodified):** Volume, as always

**Files:** `web/dsp.js` (modify), `web/app.js` (modify routing)

---

## Phase 4: Gyroscope + Expression + Polish

**Goal:** Add the expressive performance controls and refine the experience.

### 4a. Add gyroscope in `web/controls-ui.js`
- DeviceMotion API (requires HTTPS + user permission on iOS)
- **Only active when shift is held** (prevents accidental parameter changes)
- Shift+tilt = global expression macro:
  - Forward/back tilt → mapped to multiple params (reverb wet, filter cutoff, resonator decay)
  - Left/right tilt → secondary expression (e.g., chop variation, stereo width)
- Low-pass filter on gyro data for smooth control

### 4b. Double/triple click patterns
- Implement timing-based multi-click detection on all buttons
- Capture: single = capture/layer, double = undo last capture, triple = clear all
- Button 3: discover functions during prototyping

### 4c. Visual refinement
- Stage transition animations (smooth, not jarring)
- Beat-synced visual pulses (subtle glow pulse on beat)
- Slice activity visualization in chop stage
- Harmonic activity visualization in resonate stage

### 4d. PWA + persistence
- PWA manifest for "Add to Home Screen" (full-screen, no browser chrome)
- IndexedDB: save/load captured audio, settings, patterns
- Service worker for offline capability

**Files:** `web/controls-ui.js` (modify), `web/visualizer.js` (modify), `web/manifest.json` (new)

---

## Key Technical Decisions

| Decision | Rationale |
|----------|-----------|
| Web app over iOS native | Foundation exists, faster iteration, cross-platform |
| 466×466 round viewport | Exact device display size for faithful UI testing |
| No text/menus in round display | Dead-front constraint — symbols only, discover UX issues now |
| AudioWorklet for buffer + clock | Sample-accurate, no main thread blocking |
| JS resonator for iteration | Change and reload vs. Faust recompile. Port fixes to .dsp later |
| Mono processing | Halves memory, stereo unnecessary for prototype |
| Shift+gyro (not always-on) | Prevents accidental param changes from normal movement |
| Pipeline auto-advance | Capture → Chop → Resonate is one flow, not separate modes |

---

## Verification

| Phase | Test |
|-------|------|
| 1 | Open on iPhone. See round black display with glowing controls. Mic records. Capture grabs 30s. Encoder adjusts window position. Touch selects bar length. Joystick navigates. |
| 2 | After capture, transients auto-detected and visualized. Chop engine plays slices rhythmically synced to BPM. Density/variation controls reshape the pattern. |
| 3 | Chopped audio feeds resonator. Timbre slider sounds good throughout range. No unwanted distortion. Chord changes quantized to 8th notes. |
| 4 | Shift+tilt controls expression macro. Double-click capture undoes. PWA installs to home screen. Full chain works end-to-end. |
| **Full chain** | Record ambient sound → capture → auto-detect window → auto-chop → resonate → manipulate chords + expression in real time → layer additional captures. |

---

## Critical Files Summary

| File | Status | Purpose |
|------|--------|---------|
| `web/index.html` | Rewrite | Shell with round viewport + control zones |
| `web/style.css` | Rewrite | Pure black, glowing elements, round clip mask |
| `web/app.js` | New | Pipeline state machine, control routing |
| `web/device-shell.js` | New | 466×466 round display renderer |
| `web/controls-ui.js` | New | Joystick, encoder, buttons emulation |
| `web/circular-buffer-worklet.js` | New | Always-on mic recording AudioWorklet |
| `web/clock.js` | New | Sample-accurate BPM clock |
| `web/capture-intelligence.js` | New | Smart bar-aligned capture window |
| `web/transient-detector.js` | New | Spectral flux onset detection |
| `web/chop-engine.js` | New | Rhythmic slice sequencer |
| `web/visualizer.js` | New | Round display canvas visualizations |
| `web/dsp.js` | Modify | Resonator fixes (timbre, distortion, routing) |
