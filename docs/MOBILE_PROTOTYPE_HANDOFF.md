# Mobile Prototype — Implementation Handoff

**Date:** 2026-03-27
**Full plan:** `docs/MOBILE_PROTOTYPE_PLAN.md`

---

## Phase 1: Device Simulation Shell + Audio Foundation ✅ COMPLETE

**7 of 7 files implemented.**

### Completed Files (previously built)

1. **`web/circular-buffer-worklet.js`** — AudioWorklet processor
   - 30s mono ring buffer at 44100Hz (~5.3MB Float32)
   - Continuous mic input recording on audio thread
   - `capture` message → copies buffer chronologically, transfers via postMessage
   - `setLength` message → resize buffer
   - RMS level metering (~50ms intervals) sent to main thread
   - Mirrors firmware `RetroactiveBuffer` API pattern

2. **`web/clock.js`** — BPM clock (ES module, `export { Clock }`)
   - Uses AudioContext.currentTime (not setTimeout) for drift-free timing
   - Lookahead scheduling pattern (25ms check interval, 100ms schedule-ahead)
   - Events: `beat`, `eighth`, `sixteenth`, `bar` — all with time + position data
   - Methods: `getNextBeatTime()`, `getNextEighthTime()`, `getNextBarTime()`, `getCurrentBar()`, `getCurrentBeat()`
   - `quantizeToEighth(time)` for chord change quantization
   - `setQuantizeChords(bool)` toggle
   - Default 120 BPM, range 30-300

3. **`web/device-shell.js`** — Round display simulator (ES module, `export { DeviceShell, DISPLAY_SIZE }`)
   - Creates 466×466 canvas with circular clip mask
   - HiDPI support (devicePixelRatio scaling)
   - Layer-based rendering: `addLayer(renderFn)` — each layer gets `(ctx, size, timestamp)`
   - Touch event system: `onTouch(callback)` — normalized to display coords with circle boundary detection
   - Mouse fallback for desktop testing
   - requestAnimationFrame render loop

4. **`web/controls-ui.js`** — Physical control emulation (ES module, `export { createControls, ControlEvents, isShiftHeld }`)
   - **Joystick:** 5-way touch widget (up/down/left/right/center), emits `joystick` + `joystick-release` events with direction + shift state
   - **Encoder:** Circular drag gesture for turn (emits `encoder-turn` with delta/direction/shift), center tap for push (emits `encoder-push`)
   - **3 Buttons:** Capture (◉), Shift (⇧), Btn3 (◆) — multi-click detection (300ms window), emits `button` with id/clicks/shift/holdDuration. Shift sets global `_shiftHeld` flag
   - Factory: `createControls(joystickId, encoderId, buttonsId)` returns `{ events, joystick, encoder, buttons, isShiftHeld }`
   - Mouse fallback on all controls

### Completed Files (this session)

5. **`web/capture-intelligence.js`** — Smart capture window analysis (ES module, `export { CaptureIntelligence }`)
   - `loadCaptured(buffer, sampleRate)`: accepts captured Float32Array, computes waveform peaks for rendering
   - `findDefaultWindow()`: uses clock BPM to find bar-aligned boundaries (defaults to nearest 2 or 4 bars)
   - `setLengthBars(bars)`: touch selects window length (1/2/4/8 bars)
   - `nudgeBars(delta)`: encoder adjusts window position in bars
   - `snapToNearestBar()` / `confirm()`: snaps and returns sliced AudioBuffer
   - `render(ctx, size, ts)`: draws circular waveform with window bracket overlay (pulsing teal glow)
   - Idle state: animated waveform hint with pulsing ring

6. **`web/app.js`** — Pipeline controller (ES module, entry point)
   - **Pipeline state machine:** `IDLE → CAPTURE_REVIEW → CHOP → RESONATE`
   - **Audio init on first gesture:** AudioContext + all nodes created on wake-up (iOS requirement)
   - **Audio routing:** mic → resonator.inputNode (live monitoring) + mic → circular buffer worklet (capture/metering)
   - **Clock + Resonator** initialized after audioContext, before mic connection
   - **Control routing per stage:**
     - Encoder turn (unshifted): volume
     - Encoder turn (shifted): context param (timbre in RESONATE, window nudge in CAPTURE_REVIEW)
     - Encoder push: mute toggle
     - Joystick left/right: navigate pipeline stages
     - Joystick up/down: cycle bar lengths (CAPTURE_REVIEW), chord nav (RESONATE)
     - Capture button: trigger capture (IDLE) → confirm window (CAPTURE_REVIEW) → layer (CHOP/RESONATE)
     - Btn3: TBD stub (discover during prototyping)
   - **Layer swapping:** idle layer / capture review layer swap in/out based on stage
   - **Exposed:** `window.__app` (singleton), `window.__STAGE` (stage enum)
   - Stage indicator DOM element updated on stage transitions

7. **`web/index.html`** (rewrite) + **`web/style.css`** (rewrite)
   - **HTML:** Minimal shell — `#device` container with `#display-container` (round viewport), `#controls-panel` with joystick/encoder/buttons containers, `#stage-indicator` debug label. ES module script only (dsp.js removed as separate script tag).
   - **CSS:** Pure black background, dead-front aesthetic, mobile-first, no-scroll. Custom properties for glow colors. Landscape + tablet responsive. All control widgets (joystick cross, encoder ring, 3 HW buttons) styled with glowing active states. Safe-area-inset support for notched phones.

### Modified (this session)

- **`web/dsp.js`:** Complete rewrite — replaced the broken JS reimplementation with a thin adapter that wraps the real **Faust WASM DSP** from `web/faust-out/`. All Phase 1 bugs (gain `* 40` multiplier, timbre dead spots, `decay * 30` Q, missing Zita reverb) are gone.
- **`web/dsp.js`** now imports `createFaustNode` from `./faust-bridge.js` (copied from `faust-out/`), compiles `dsp-module.wasm`, and exposes the same `MorphingResonator` class interface that `app.js` expects: `init()`, `setParam()`, `pluck()`, `params`, `inputNode`, `enableMic()`, `disableMic()`, `resume()`.

### Copied from `web/faust-out/` (this session)

| File | Role |
|------|------|
| `web/faustwasm/index.js` + `index.d.ts` + `index.js.map` | Faust WASM JS bindings |
| `web/dsp-module.wasm` | Compiled MorphingResonator DSP binary (~960KB) |
| `web/dsp-meta.json` | Faust parameter addresses + UI metadata |
| `web/faust-bridge.js` | `createFaustNode()` — WASM compile + node creation |

---

## What Remains — Phase 1 Verification

Phase 1 is fully implemented. Remaining step: **serve locally and test on device** to verify:
1. Round display renders with pulsing ring in IDLE
2. Controls are touchable and emit events
3. First tap/click initializes audio (mic permission prompt)
4. Circular buffer records 30s of mic input
5. Capture button grabs buffer and shows waveform in CAPTURE_REVIEW
6. Encoder adjusts volume
7. Joystick navigates stages

Serve with: `cd web && python3 -m http.server 8080`

---

## Phases 2-4 (All Future)

| Phase | Files to Create | Status |
|-------|----------------|--------|
| **2: Transient Detection + Chop** | `transient-detector.js`, `chop-engine.js`, `visualizer.js` | Not started |
| **3: Resonator Fixes + Integration** | Modify `dsp.js` (timbre fix, distortion fix, routing) | Not started |
| **4: Gyroscope + Polish** | Modify `controls-ui.js`, add PWA manifest | Not started |

---

## Known Issues / Bugs (Deferred to Phase 3)

### Resonator Distortion Root Cause (in `web/dsp.js`)
- **Line 265-266:** `* 40` gain multiplier on engine outputs doesn't exist in Faust source (`resonatorTest2.dsp` line 71). This is a **40x gain difference** and the primary distortion cause.
- **Line 97:** `* 4` per-filter gain multiplier compounds the issue.
- **Line 233:** `decay * 30` Q value is too aggressive — should be `decay * 15`.
- Fix in Phase 3.

### Timbre Slider Dead Spots (in `web/dsp.js`)
- **Line 243:** Linear frequency blending `harmF * w0 + inharmF * w1 + fixedF * w2` creates bad-sounding frequencies at non-integer timbre values.
- Fix: interpolate in log space, use equal-power crossfade weights.
- Fix in Phase 3.

---

## Architecture Notes

- All new JS files use **ES modules** (`export`/`import`). The HTML uses `<script type="module">`.
- **`web/dsp.js`** is now a **Faust WASM adapter** — not a JS reimplementation. The real DSP lives in `dsp-module.wasm`. The adapter wraps it with the same `MorphingResonator` class interface the pipeline expects.
- **`web/faust-out/`** is the standalone resonator development app (slider UI, SVG chord ring). It is kept in sync with the Phase 1 prototype via the shared `dsp-module.wasm` binary. Changes to the Faust `.dsp` source must be recompiled and copied to both `web/` and `web/faust-out/`.
- The circular buffer worklet is loaded via `audioContext.audioWorklet.addModule('circular-buffer-worklet.js')`.
- The `Clock` class takes an `AudioContext` as constructor arg — it doesn't create its own.
- The `DeviceShell` manages a render loop; other modules register render layers via `addLayer()`.
- Controls emit events via a shared `ControlEvents` instance — the app controller subscribes to these.
- The `isShiftHeld()` function is exported from controls-ui.js as a global query.
- `dsp.js` is now an ES module (added `export { MorphingResonator }`). Do NOT load it as a classic `<script>` tag.

---

## Interview Decisions (Key UX Choices)

These were decided during the design interview and should be treated as requirements:

1. **One pipeline, not separate modes** — Capture → Chop → Resonate auto-advances as one flow
2. **3 buttons = Capture + Shift + TBD** — Shift is a modifier key for all controls
3. **Encoder = volume by default, shift+turn = context parameter per stage**
4. **7-8 diatonic chords** as larger touch targets (future: dynamic harmonic compass)
5. **Shift+gyro only** — gyro inactive without shift held, controls global expression macro
6. **Dead-front: pure black + colored glowing icons** — no text, no menus, no background colors
7. **Capture window: encoder = position, touch = bar length**
8. **Double/triple clicks** for secondary button functions
9. **Chord changes quantized to 8th notes** by default (toggleable)
