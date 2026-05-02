# Drum + Synth Engine Design Notes

**Date:** 2026-05-02
**Status:** Pre-implementation design decisions, captured for reference. No code yet.

These notes record the early design decisions for the drum sample engine and simple synth engine described in `docs/PRD.md` §3. They precede formal implementation issues and exist so the choices are not relitigated later.

---

## Drum Sample Engine

### Stage placement
Own dedicated stage (`STAGE_DRUMS`), not bolted onto CHOP. Reason: chop is for the captured audio loop; drums are an additive layer with their own UI surface needs (lane selection, step grid, kit picker).

### Engine basics
- Sample playback engine, 8 simultaneous voices (target)
- 16-step sequencer per drum lane (kick / snare / hat-closed / hat-open / clap / perc1 / perc2 / perc3 typical)
- Euclidean rhythm generation reuses Bjorklund algorithm from `chop_engine` — same code, applied per-lane

### Retroactive capture (4th capture type)
Drums fit the universal capture paradigm naturally:

| Stage | Capture buffer |
|---|---|
| CAPTURE | audio |
| CHOP | gesture |
| RESONATE | chord progression |
| **DRUMS** | step pattern + sample triggers |

Drum capture is a control-event loop (not audio), so it costs nothing in CPU or memory. Same btn 1, same 4 s post-capture adjust ribbon, same paradigm.

### Voice stealing
**Oldest voice wins** when an 8-voice limit is hit. Standard sampler behavior; predictable, no surprise mutes.

### Sample storage
- ~768 KB per typical 12-sample kit at 32 kHz (16-bit mono)
- Kits stored on SD card, loaded into PSRAM at kit-select time
- Multiple kits selectable via web config (#17) or in-stage encoder gesture
- Source samples typically 44.1 kHz; resampled to 32 kHz at load time (offline, no runtime cost)

### Reuse of the `SampleSource` abstraction (#28)
The drum engine MUST reuse the `SampleSource` interface introduced for the BREAK stage in #28 rather than rolling its own sample-loading path. Concretely:
- Each loaded drum hit is a `DiskSampleSource` instance
- Slice points for a multi-hit kit file (if used) read through the same offline `.slices` sidecar pattern
- The on-disk WAV + `.slices` schema is shared between BREAK and DRUMS so there is one sample format, one loader, one analyzer
- Voices in the drum engine each hold a `SampleSource*` pointer; voice stealing replaces the pointer

This avoids duplicating sample I/O code, ensures the web config app's sample library management (#17) covers both stages, and makes it possible later for drums and BREAK to share kit assets transparently.

### CPU budget at 32 kHz
8 voices of sample playback + ADSR envelope: ~5 % of one Core 1 core. Very light.

### Open implementation questions (defer to implementation issue)
- In-stage UI for step grid editing (joystick to navigate cells, encoder to adjust velocity?)
- Per-lane swing inheritance from global swing (#5) vs. per-lane override
- Optional drum-bus filter / saturation as a built-in effect

---

## Simple Synth Engine

### Stage placement
Own dedicated stage (`STAGE_SYNTH`) — same reasoning as drums. Likely sits between RESONATE and DRUMS in the joystick L/R nav order.

### Pitch quantization
**Always quantized to the active resonator scale by default.** Hard to use musically otherwise on a small device with no piano keyboard. A "free pitch" toggle is exposed in the web config app (#17) for power users who want microtonal / out-of-scale exploration.

The synth reads the same chord index → root note mapping the resonator uses (`scales.cpp`), so a melody played on the synth is automatically in key with the resonator chord and the loop.

### Voice modes
- Mono (default for melody) — target first
- 4-voice poly (target for chords / arpeggios)
- Voice limit configurable in web config

### Oscillator types
TBD during implementation. Roadmap, simplest first:
1. **Sine** — trivial CPU, no aliasing
2. **Wavetable** — cheap; needs anti-alias mip-mapping for high notes
3. **2-op FM** — moderate CPU; needs care above 8 kHz fundamental at 32 k SR
4. **Subtractive (band-limited saw/square + filter)** — heavier; polyBLEP for clean oscillators

Initial implementation likely sine + simple wavetable. FM and subtractive are stretch goals.

### Pitch input source
TBD per stage UI design. Candidates:
- Gyro Y axis (tilt for pitch, while shift held)
- Touch X (drag for pitch on a dedicated touch surface in SYNTH stage)
- Step sequencer per voice (similar to drums but pitched)

Likely a mix: touch for live melody input, sequencer for loops.

### Voice stealing
**Oldest voice wins.** Same as drums.

### Retroactive capture
**Own buffer**, not shared with the CHOP gesture buffer. Reasons:
- Synth note events are a different event type (note-on with pitch + velocity, note-off, optional pitch-bend) than chop gestures (touch XY, encoder ticks)
- Mixing them in one buffer would require a tagged-union event format that complicates replay logic
- Each capture-able domain (audio / chop gesture / chord progression / drum pattern / synth notes) gets its own ring buffer; the universal capture button (#2) routes to whichever buffer is active for the current stage

### CPU budget at 32 kHz
- 4-voice mono sine: ~3 %
- 4-voice poly wavetable: ~12 %
- 4-voice poly subtractive with polyBLEP + 1 filter per voice: ~20 %
- 4-op FM 4-voice: ~25 %

If the budget tightens with all engines running, the synth poly path can run at SR/2 (16 kHz) internally and upsample to 32 kHz for the master mix — halves cost while still beating native 16 k aliasing (oscillator design at 16 k internal can use much steeper anti-alias than codec rate would allow).

---

## Shared concerns

### Tempo source
Both engines drive off `BPMClock_onSixteenth`. Single shared tempo. BPM control lives on CHOP page 2 (see separate issue) since rhythm-feel parameters cluster there.

### Core assignment
Both run on Core 1 (audio task). Core 0 stays for UI / display / touch / IMU.

### Sample rate
32 kHz preferred for both. Drums need it for cymbal / hi-hat top end; synth needs it for oscillator anti-aliasing headroom.

### Web config integration (#17)
- Per-engine voice limits
- Synth pitch-quantize bypass
- Default kit / waveform selection
- Per-lane drum patterns as named presets
- Live CPU meter so users can see when they're approaching the budget

### Voice budget arithmetic at 32 kHz (Core 1, worst-case)

| Block | % of one core |
|---|---|
| Resonator (5-voice + Freeverb) | ~30 |
| Dual chop (#11) | ~10 |
| Modulation matrix (#9) | ~2 |
| Drums (8 voices) | ~5 |
| Synth (4 voices subtractive) | ~20 |
| DC blocker / HPF / pre-emphasis / limiter | ~5 |
| Transient detector | ~3 |
| I2S DMA + interrupts | ~10 |
| **Worst-case total** | **~85** |

Tight but workable. Realistic concurrent usage closer to 60–75 % since granular and chop are mutually exclusive, drum kit rarely fires all 8 voices, and synth rarely uses full 4-voice poly. Mitigations available if pressure grows: voice limits, internal SR/2 synth path, ESP-DSP intrinsics, dynamic polyphony degradation.

---

## Decisions captured here, not yet filed as issues

- Drum sample engine implementation (deferred until closer to build)
- Simple synth engine implementation (deferred)
- Drum step-pattern retroactive capture (extends #4 paradigm; will be covered by drum engine issue when filed)
- Synth note retroactive capture (own buffer; will be covered by synth engine issue)
