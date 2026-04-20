# Plan: Musical Framework Mobile Prototype v2

## Context

This is the updated implementation plan for the Tombogo x Rheome Collab Gadget mobile prototype. It supersedes the original MOBILE_PROTOTYPE_PLAN.md.

**What changed from v1:**
- Completed: Phases 1, 2, and most of Phase 3 (chord UI, routing, quantization)
- New: Chord loop capture stage (Phase X)
- New: Reverse + ratchet slice playback variants
- New: Dynamic chord response to chop timbre
- New: Sample repitch in capture review
- Revised: Shift encoder = volume (always), unmodified encoder = context-dependent
- Remaining: Phase 3 resonator fixes, Phase 4 gyroscope + PWA, downbeat indicator

---

## Revised Physical Controls

| Physical Control | Web Emulation | Default Function | Shift+ Function |
|-----------------|---------------|------------------|-----------------|
| **Capture button** | Dedicated on-screen button | Capture (always). 2nd press = layer | Shift+Capture = clear layer |
| **Shift button** | Dedicated on-screen button (hold) | Modifier for all other controls | — |
| **Button 3** | On-screen button | TBD / context-dependent | — |
| **5-way joystick** | Touch joystick widget | Left/Right = navigate pipeline. Up/Down = context in stage. Center = primary action | Shift+joystick = extended nav |
| **Rotary encoder** | Touch ring/drag gesture | **Context-dependent** (pitch in capture, density in chop, timbre in resonate) | **Always volume** |
| **Touchscreen (round)** | 466×466 round viewport | Chord selection, waveform interaction, X-Y pad | Shift+touch = extended gestures |
| **Gyroscope/IMU** | DeviceMotion API | Inactive (prevents accidental changes) | Shift+tilt = global expression macro |

### Revised Encoder Mapping (v2)

| Stage | Unmodified Encoder | Shift+Encoder |
|-------|-------------------|----------------|
| IDLE | (no effect) | Volume |
| CAPTURE_REVIEW | Pitch (semitones, -12 to +12) | Volume |
| CHOP | Density (0–1) | Volume |
| RESONATE | Timbre morph (0–3) | Volume |
| CHORD_LOOP | Loop length (bars) | Volume |

---

## Revised Pipeline Stages

```
[CAPTURE] → [CAPTURE_REVIEW] → [CHOP] → [RESONATE] → [CHORD_LOOP]
                ↓
           [pitch via encoder]
                ↓
           [timbre via shift+gyro]
```

IDLE is the always-on listening state. Capture → CAPTURE_REVIEW (window adjust) → CHOP → RESONATE → CHORD_LOOP. Joystick left/right navigates. Any stage can return to IDLE (shift+capture).

---

## Phase R: Resonator Fixes (Remaining Phase 3)

**Goal:** Fix timbre slider and distortion before they block iteration.

### R.1 Fix timbre slider in `web/dsp.js`
**Problem:** Linear frequency blending between harmonic modes creates dead spots.
**Fix:**
- Interpolate in **log space**: `freq = Math.exp(Math.log(harmF)*w0 + Math.log(inharmF)*w1 + Math.log(fixedF)*w2)`
- Equal-power (cosine) crossfade weights instead of linear triangle
- Per-harmonic gain rolloff: `1/sqrt(h+1)`

### R.2 Fix distortion in `web/dsp.js`
**Root cause:** JS has `* 40` gain multiplier on engine outputs (lines 265-266) that doesn't exist in Faust source. **40x gain difference.**
**Fix:**
- Remove `* 40` from engine gain calculations
- Remove `* 4` per-filter gain (line 97)
- Reduce Q: `decay * 15` instead of `* 30`
- Adjust wet trim default upward to compensate

### R.3 High-Pass Filter (DC Block / Rumble Removal)

**Problem:** Physical modeling synthesis (especially resonators with high feedback/decay) can generate sub-audio "rumble" or DC offsets that eat headroom and cause distortion.
**Fix:**
- Add a 2nd-order High Pass Filter (HPF) at 100Hz to the resonator signal path.
- **Option A (DSP):** Update Faust source to include `fi.highpass(2, 100)` before the output.
- **Option B (Web Audio):** Insert a `BiquadFilterNode` (`type: "highpass"`) in `web/app.js` between the resonator output and the `_wetMakeup` gain node.
- **Default:** Option B for rapid iteration without re-compiling WASM.

**Files:** `web/app.js` (modify routing)

**Files:** `web/dsp.js` (modify)

---

## Phase N: New Features

**Goal:** Add the new requested features from user testing.

### N.1 Reverse + Ratchet Slice Playback (Chop Engine Enhancement)

**What:** When center joystick randomizes the chop pattern, individual slice playbacks have a chance of being reversed or ratcheted.

**Playback variants:**
- `normal` — slice plays as recorded (default)
- `reverse` — slice plays backwards (~15% probability on random)
- `ratchet` — slice plays 2–3× with tight tremolo envelope (~8% probability on random)

**Implementation:**
```javascript
// In ChopEngine._playSlice(), add per-step playbackType
const PLAYBACK_TYPES = ['normal', 'normal', 'normal', 'normal',  // heavily weighted normal
                        'reverse', 'normal', 'normal',
                        'ratchet', 'normal', 'reverse'];

_playbackTypeForStep(step) {
  const r = Math.random();
  if (r < 0.08) return 'ratchet';
  if (r < 0.23) return 'reverse';
  return 'normal';
}
```

- `reverse`: `src.playbackRate.value = -1`, start from slice end
- `ratchet`: replay slice 2–3× in tight loop with amplitude tremolo (50ms on/off) for a "dub siren" effect

**Visual:** Reverse slices render with a different glow color (cooler blue tint) in the visualizer. Ratchet slices pulse.

**Files:** `web/chop-engine.js` (modify `_playSlice`, `randomize`)

---

### N.2 Dynamic Chord Response to Chop Timbre

**What:** The resonator chord responds to the spectral character of each incoming chop slice, creating a dynamic voicing that follows the source texture.

**Two modes (user-selectable via Button 3 in RESONATE):**

**Mode A — Timbre-Split Voicing:**
- Each chop slice is analyzed for spectral centroid (FFT on the slice region, ~10ms)
- Low centroid (bass-heavy slice) → chord root or 5th inversion
- High centroid (treble-heavy slice) → 3rd or upper octave voicing
- The chord index shifts by ±1–2 steps based on centroid band

**Mode B — Chord Arpeggiator:**
- Each chop trigger advances to the next chord tone in the current chord
- Cycles through: root → 3rd → 5th → 7th → root
- Pattern is deterministic but shuffled each cycle reset

**Implementation:**
```javascript
// In app.js, subscribe to chop 'slice' events
_chopEngine.on((event, data) => {
  if (event === 'slice' && this._stage === STAGE.RESONATE) {
    const centroid = this._computeSpectralCentroid(data.sliceIdx);
    const voicingOffset = this._centroidToVoicingOffset(centroid);
    this._resonator.setParam('voicingOffset', voicingOffset);
  }
});
```

**Spectral centroid:** Run FFT on slice region of the captured AudioBuffer (offline, pre-computed when transient detection runs). Store per-slice centroid in a Float32Array.

**Files:** `web/chop-engine.js` (modify `loadSlices` to compute centroid), `web/app.js` (add chord response logic), `web/visualizer.js` (add voicing indicator)

---

### N.3 Resonate Chord Loop Capture (New Stage: CHORD_LOOP)

**What:** User plays a chord progression in RESONATE, presses capture, and the chord changes loop independently of the audio loop.

**Architecture shift:** Resonate capture loops *control signals* (MIDI-like chord events), not audio. The underlying audio loop continues unchanged underneath.

**Stages:**
1. User is in RESONATE, playing chords
2. Press capture button → enters CHORD_LOOP_REVIEW
3. Chord event log is displayed as glowing dots around the ring (one per chord change)
4. User adjusts window length (1/2/4 bars) with encoder
5. User adjusts window position (nudge start) with shift+encoder
6. Press capture again to confirm → chord loop plays back
7. Press right joystick to return to RESONATE (chord loop keeps playing)
8. Press left joystick → stop chord loop, return to RESONATE with live chord

**Chord event log:**
```javascript
{
  bar: number,      // absolute bar number
  beat: number,     // 1-4
  chordIdx: number, // 0-7 (diatonic)
  rootNote: number, // MIDI root
}
```

**Capture intelligence:**
- Track time between chord changes while in RESONATE
- If user changes chords frequently (>1 change per 2 bars) → default capture window = 4 bars
- If sparse changes (<1 per 4 bars) → default = 2 bars
- Use clock `getCurrentBar()` to snap window to bar boundaries

**Downbeat indicator:**
- Subtle white flash on the outer ring of the round display at bar 1, beat 1
- Frequency: every bar (on beat 1 of each bar at the current BPM)
- Visual: 80ms pulse, 30% alpha white arc at display edge

**Chord loop playback:**
- Maintains its own clock subscriber (not the chop clock — a dedicated chord clock)
- On each chord event in the loop, calls `resonator.setParam('chordIdx', ...)`
- Chord changes are quantized to the next beat boundary
- Mutes live chord selection while loop is playing (chord changes from loop only)

**Files:** `web/app.js` (add CHORD_LOOP stage, chord event log, chord loop player), `web/visualizer.js` (add chord loop review layer), `web/clock.js` (add bar events if not present)

---

### N.4 Sample Repitch in Capture Review

**What:** The unmodified encoder in CAPTURE_REVIEW controls playback pitch in semitones.

**Implementation:**
- `AudioBufferSourceNode.playbackRate = 2^(semitones/12)`
- Range: -12 to +12 semitones (1 octave down to 1 octave up)
- Quantize changes to avoid zipper noise (use `setTargetAtTime` with 10ms time constant)
- Fine-tune: shift+encoder nudges by ±1 semitone (coarse = encoder turns)

**Display:** Show semitone offset as a glowing arc around the waveform, or as a subtle ±N label near the waveform center.

**Files:** `web/app.js` (modify `_setupCaptureReviewControls`, add pitch parameter), `web/visualizer.js` (add pitch arc indicator)

---

## Phase 4: Gyroscope + Expression + Polish (from v1)

**Goal:** Add expressive performance controls and refine the experience.

### 4a. Add gyroscope in `web/controls-ui.js`
- DeviceMotion API (requires HTTPS + user permission on iOS)
- **Only active when shift is held**
- Shift+tilt = global expression macro:
  - Forward/back tilt → reverb wet + filter cutoff + resonator decay
  - Left/right tilt → chop variation + stereo width
- Low-pass filter on gyro data (alpha = 0.1) for smooth control

### 4b. Double/triple click patterns
- Capture: single = capture/layer, double = undo last capture, triple = clear all
- Button 3: discover functions during prototyping (chord response mode toggle from N.2)
- Fix current bug: capture requires two taps on fresh session

### 4c. Visual refinement
- Beat-synced pulse on round display edge (subtle white flash every beat)
- Slice activity visualization in chop stage (active slice glows)
- Voicing indicator in resonate stage (which chord inversion is active)
- Stage transition animations (fade, not jump)
- **Downbeat indicator** (white flash on bar 1, see N.3)

### 4d. PWA + persistence
- PWA manifest for "Add to Home Screen" (full-screen, no browser chrome)
- IndexedDB: save/load captured audio, settings, chord loop patterns
- Service worker for offline capability

**Files:** `web/controls-ui.js` (modify gyroscope), `web/visualizer.js` (modify beat pulse, downbeat), `web/manifest.json` (new), `web/sw.js` (new)

---

## Implementation Order

### Immediate (Next Session)
1. ✅ ~~R.1 + R.2~~ — Resonator fixes (timbre/gain in compiled Faust WASM; dsp.js is now pure wrapper)
2. ✅ ~~N.4~~ — Sample repitch in CAPTURE_REVIEW (encoder → semitones, pitch arc rendered)
3. **4b fix** — Fix capture double-tap bug

### Short Term
4. ✅ ~~N.1~~ — Reverse + ratchet slice playback (already implemented per git history)
5. **4a** — Gyroscope with shift+tilt expression macro
6. **4c** — Downbeat indicator + visual polish

### Medium Term
7. **N.3** — Resonate chord loop capture (new stage)
8. **N.2** — Dynamic chord response to chop timbre

### Later
9. **4d** — PWA manifest + service worker

---

## Key Technical Decisions

| Decision | Rationale |
|----------|-----------|
| Shift encoder = volume always | Cleaner mental model: volume is global, context is local |
| Resonate captures control signals, not audio | Loop audio is already captured; chord progression is a separate concern |
| Chord loop has its own clock subscriber | Independent of chop engine, can play while chop is active or muted |
| Spectral centroid computed offline | No real-time cost; one FFT per slice at transient detection time |
| Ratchet = tremolo replay, not time-stretch | Simpler to implement, more dramatic sonic character |
| Pitch quantized via setTargetAtTime | Avoids zipper noise on rapid encoder turns |

---

## Verification

| Feature | Test |
|---------|------|
| Resonator fixes | Timbre slider sounds smooth across full range. No sudden loudness jumps. |
| Sample repitch | Encoder in CAPTURE_REVIEW shifts pitch. No zipper noise. ✅ Implemented |
| Capture no longer needs double-tap | Single press of init button then capture works |
| Reverse slices | Center joystick randomizes, ~15% of slices play backwards (audibly flipped transients) |
| Ratchet slices | ~8% of slices play tremolo replay |
| Gyroscope expression | Shift held + tilt changes reverb/decay/filter simultaneously |
| Downbeat indicator | Subtle white flash at bar 1 on round display edge |
| Chord loop capture | Press capture in RESONATE → chord log displayed → adjust window → confirm → chord loop plays |
| Dynamic chord voicing | Spectral centroid of each slice shifts chord inversion (Mode A) |
| Chord arpeggiator | Each chop trigger advances to next chord tone (Mode B) |
| PWA | App installs to iOS home screen, full-screen, offline |

---

## Consolidated File List

| File | Status | Purpose |
|------|--------|---------|
| `web/index.html` | ✅ Done | Shell with round viewport + control zones |
| `web/style.css` | ✅ Done | Pure black, glowing elements, round clip mask |
| `web/app.js` | ✅ Done | Pipeline state machine, control routing — add CHORD_LOOP stage; N.4 pitch encoder done |
| `web/device-shell.js` | ✅ Done | 466×466 round display renderer |
| `web/controls-ui.js` | ✅ Done | Joystick, encoder, buttons emulation — add gyroscope |
| `web/circular-buffer-worklet.js` | ✅ Done | Always-on mic recording AudioWorklet |
| `web/clock.js` | ✅ Done | Sample-accurate BPM clock — add downbeat events |
| `web/capture-intelligence.js` | ✅ Done | Smart bar-aligned capture window — add pitch arc for N.4 |
| `web/transient-detector.js` | ✅ Done | Spectral flux onset detection — add spectral centroid |
| `web/chop-engine.js` | ✅ Done | Euclidean rhythm sequencer — add reverse/ratchet |
| `web/visualizer.js` | ✅ Done | Round display visualizations — add voicing + downbeat |
| `web/dsp.js` | ✅ Done (v2 WASM) | Timbre/gain now in compiled Faust WASM — dsp.js is pure wrapper |
| `web/manifest.json` | 🔴 New | PWA manifest |
| `web/sw.js` | 🔴 New | Service worker |
