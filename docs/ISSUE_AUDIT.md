# Issue Audit & Implementation Plan

**Date:** 2026-05-02
**Scope:** All 37 open GitHub issues at `rheome-dev/tombogoCollabGadget` as of writing
**Purpose:** Map dependencies, grade implementation + testing complexity, identify which issues can be safely handed off to a coding agent versus which need close human supervision.

---

## Rubric

Each issue is graded on three axes. Combine them to read agent-suitability.

### Implementation complexity

| | Description |
|---|---|
| 🟢 | < 1 day, isolated change, well-scoped, mostly known coefficients/values |
| 🟡 | 1–3 days, modest refactor, moderate scope, multiple files |
| 🔴 | Week+, architectural change, touches multiple subsystems |

### Testing complexity

| | Description |
|---|---|
| 🟢 | Objectively verifiable: build passes, log shows expected register values, file written correctly |
| 🟡 | Hardware-in-loop but structured: listen for clicks, run a script, check meter values |
| 🔴 | Subjective sonic evaluation, fine-grained gesture-feel testing, false-trigger tuning, edge cases hard to enumerate |

### Breaking-change risk

| | Description |
|---|---|
| 🟢 | Additive or gated; broken state easily revertible; isolated from working code |
| 🟡 | Changes user-visible behavior in a predictable, bounded way |
| 🔴 | Touches the core audio loop, state machine, or boot path; failures may be silent or audible-but-bad |

### Hands-off-agent suitability

| Profile | Recommendation |
|---|---|
| 🟢🟢🟢 or 🟢🟡🟢 | **Hands-off candidate.** Brief; agent can implement and verify mostly autonomously. Human spot-checks the build + listens once. |
| 🟡🟡🟢 or 🟢🔴🟡 | **Light supervision.** Agent implements; human listens through the change carefully and can revert easily. |
| Any 🔴 | **Active supervision.** Agent should pair with human review at architecture, mid-implementation, and post-implementation. |

---

## Bug fixes that block usability — file under their own banner

| # | Title | Impl | Test | Risk | Notes |
|---|---|:---:|:---:|:---:|---|
| #35 | Touch alignment / tap reliability | 🟡 | 🟡 | 🟡 | RESONATE chord cluster needs multiple taps; root cause likely transform + I²C reliability. Instrument first, then fix. **Block on this before any deeper touch UI work.** |

---

## Quick wins — hands-off-agent ready (start here)

These are pure-fix issues where the change is small, the verification is structured, and the rollback is trivial. An agent can implement these confidently with light human review.

| # | Title | Impl | Test | Risk | Notes |
|---|---|:---:|:---:|:---:|---|
| #18 | DAC volume cap at 0 dB | 🟢 | 🟡 | 🟢 | One-line change with known register value. Highest leverage / lowest risk. **Land first.** |
| #19 | DAC ramp + 0 dB default | 🟢 | 🟡 | 🟢 | 3-line init addition. Pairs with #18. |
| #20 | 100 Hz Butterworth HPF | 🟢 | 🟡 | 🟡 | Coefficients given; replace existing DC blocker block. |
| #23 | Fix L+R sum overflow ordering | 🟢 | 🟡 | 🟢 | Arithmetic refactor; net gain unchanged. |
| #5 | Wire swing/shuffle | 🟢 | 🟡 | 🟢 | One float in `bpm_clock`. Stub enums already in place. |
| #6 | Polymetric chop lengths | 🟢 | 🟡 | 🟢 | `state.steps` becomes user-selectable. Bjorklund handles arbitrary lengths. |
| #8 | Per-step probability | 🟢 | 🟡 | 🟢 | Flag check in step playback + encoder gesture wiring. |
| #27 | BPM control via encoder | 🟢 | 🟡 | 🟢 | Wires existing tap-tempo + setBPM into UI. Depends on #1 + #3 landing. |
| #29 | Auto-save captures to SD | 🟢 | 🟡 | 🟢 | WAV writer + path scheme. Depends on #2. |
| #24 | Lower resonator wet_trim to 1.0 | 🟢 | 🟡 | 🟢 | One constant; **must follow #18.** |

---

## Light supervision — agent implements, human listens carefully

These are well-defined but touch the audio path or have subjective verification needs. An agent can implement, but human listens through the result before merging.

| # | Title | Impl | Test | Risk | Notes |
|---|---|:---:|:---:|:---:|---|
| #21 | 4 kHz LPF + cubic soft-clip in resonator | 🟡 | 🔴 | 🟡 | Faust regen needed. Coefficients from Wingie 2 are known-good but the resulting timbre is a judgment call. |
| #22 | ES7210 hi-res RX (recover bottom 8 bits) | 🟢 | 🔴 | 🟡 | Gated by `ES7210_HIRES_RX` flag, A/B-able. Verifying the noise-floor improvement requires careful listening with reference content. |
| #25 | Lookahead limiter (64-sample) | 🟡 | 🔴 | 🟡 | Replaces critical-path limiter. New code path; needs latency + transient testing. |
| #26 | Anti-denormal injection | 🟡 | 🟡 | 🟢 | Patches Faust-generated C++. Verification = CPU profiling at note tail. |
| #7 | Reichian gradual mutation | 🟡 | 🟡 | 🟡 | Replaces 8-bar wholesale rotation. Density invariant must be preserved. |
| #13 | RESONATE p2 XY morph pad | 🟡 | 🟡 | 🟢 | Touch routing + Faust param call. Verification = drag-test feel. |
| #30 | Project save/load | 🟡 | 🟡 | 🟡 | JSON schema design + state restore. Round-trip testable. Schema versioning is the trap to watch. |
| #4 | Retroactive gesture capture | 🟡 | 🔴 | 🟡 | Replay timing must be sample-accurate. Subtle drift is the failure mode. |
| #10 | Granular read mode | 🟡 | 🔴 | 🟡 | New playback algo. Sonic quality (grain artifacts, click-free overlap) is the judgment call. |
| #11 | Dual simultaneous chop voices | 🟡 | 🔴 | 🟡 | Exploratory; flagged as needing busyness mitigation review before defaulting on. |
| #15 | Mic-driven reactive variation | 🟡 | 🔴 | 🟡 | False-trigger tuning is the work. PA bleed protection is the risk. |
| #33 | Mixer overlay (deferred) | 🟡 | 🟡 | 🟡 | Light overlay UI. Deferred until 2+ engines run simultaneously. |
| #3 | Button mapping refactor (semantic events) | 🟡 | 🟡 | 🟡 | Touches every input handler. Migration is mechanical but needs full sweep. |
| #1 | Page-based stage navigation | 🟡 | 🟡 | 🟡 | Touches every stage handler + adds page-dot widget. Foundational for many later issues. |
| #36 | Capture-window dead-front UI | 🟡 | 🟡 | 🟢 | LVGL `capture_ribbon` widget. Pairs with #2; can be implemented alongside or just before. |
| #37 | Euclidean chop dot ring | 🟡 | 🟡 | 🟢 | LVGL `chop_ring` widget. Reusable across CHOP and BREAK (#28). Display only; no audio risk. |

---

## Active supervision required — pair with human review

Architectural changes, deeply subjective work, or core-loop touches where mistakes are audible/destructive. Don't hand these to an unsupervised agent.

| # | Title | Impl | Test | Risk | Why it needs eyes on it |
|---|---|:---:|:---:|:---:|---|
| #16 | Sample rate bump to 32 kHz | 🟡 | 🔴 | 🔴 | MCLK divider, codec re-tune, Faust regen, 6+ SR-dependent constants to audit. Failures can be silent (wrong pitch) or boot-fatal. Validate CPU + listen to A/B pre-merge. |
| #2 | Universal capture + adjust ribbon | 🔴 | 🔴 | 🔴 | Touches every stage's capture flow. The 4-second adjust UX is gesture-timing-sensitive and load-bearing for many later issues. |
| #9 | Cross-engine modulation matrix | 🔴 | 🔴 | 🟡 | New LFO engine + routing primitive + RESONATE p3 UI. Many surface areas; LFOs must lock to BPMClock without drift. |
| #12 | CHOP touch press-to-stutter | 🟡 | 🔴 | 🟡 | Six gestures must all release cleanly without click. Tape-stop ramp recovery is the trap. |
| #14 | CAPTURE p2 vinyl-scratch scrubber | 🔴 | 🔴 | 🔴 | Variable-rate playback + cubic interp + rate-dependent LPF + LVGL animation. Aliasing + click-on-release are the failure modes. |
| #28 | BREAK stage with `SampleSource` abstraction | 🔴 | 🔴 | 🟡 | Refactors the chop engine's read path; introduces a new stage; ships with sample browser + retiming. Mostly additive, but the abstraction must be right because #34 (DRUMS later) reuses it. |
| #31 | Multitrack stem export | 🔴 | 🔴 | 🔴 | Refactors the mix path; per-engine output rings + recorder task; SD write-burst handling. Touches the audio-engine inner loop. |
| #34 | SCENE stage | 🔴 | 🔴 | 🔴 | Requires every engine to expose `set_state(json)` for atomic restore. Bar-quantized scene transitions cannot glitch audio. |
| #17 | Web config app over BLE/WiFi | 🔴 | 🔴 | 🟡 | New comms subsystem (BLE GATT + WiFi softAP/STA) + companion web SPA. Massive surface; roll out in phases via sub-issues. |
| #32 | Web app feature parity tracker | 🔴 | 🔴 | 🟢 | Ongoing milestone, not a single PR. Real work happens in sub-issues. |

---

## Full grading table (sortable summary)

| # | Title | Impl | Test | Risk | Suitability |
|---|---|:---:|:---:|:---:|---|
| 1 | Page-based stage navigation | 🟡 | 🟡 | 🟡 | Light supervision |
| 2 | Universal capture + adjust ribbon | 🔴 | 🔴 | 🔴 | Active supervision |
| 3 | Button mapping (semantic events) | 🟡 | 🟡 | 🟡 | Light supervision |
| 4 | Retroactive gesture capture | 🟡 | 🔴 | 🟡 | Light supervision |
| 5 | Wire swing/shuffle | 🟢 | 🟡 | 🟢 | **Hands-off** |
| 6 | Polymetric chop lengths | 🟢 | 🟡 | 🟢 | **Hands-off** |
| 7 | Reichian mutation | 🟡 | 🟡 | 🟡 | Light supervision |
| 8 | Per-step probability | 🟢 | 🟡 | 🟢 | **Hands-off** |
| 9 | Cross-engine modulation matrix | 🔴 | 🔴 | 🟡 | Active supervision |
| 10 | Granular read mode | 🟡 | 🔴 | 🟡 | Light supervision |
| 11 | Dual chop voices (exploratory) | 🟡 | 🔴 | 🟡 | Light supervision |
| 12 | CHOP press-to-stutter touch | 🟡 | 🔴 | 🟡 | Active supervision |
| 13 | RESONATE p2 morph pad | 🟡 | 🟡 | 🟢 | Light supervision |
| 14 | CAPTURE p2 vinyl scrubber | 🔴 | 🔴 | 🔴 | Active supervision |
| 15 | Mic-driven reactive variation | 🟡 | 🔴 | 🟡 | Light supervision |
| 16 | 32 kHz sample rate bump | 🟡 | 🔴 | 🔴 | Active supervision |
| 17 | Web config app | 🔴 | 🔴 | 🟡 | Active supervision |
| 18 | DAC volume cap | 🟢 | 🟡 | 🟢 | **Hands-off** |
| 19 | DAC ramp + 0 dB default | 🟢 | 🟡 | 🟢 | **Hands-off** |
| 20 | 100 Hz HPF | 🟢 | 🟡 | 🟡 | **Hands-off** |
| 21 | 4 kHz LPF + cubic soft-clip | 🟡 | 🔴 | 🟡 | Light supervision |
| 22 | ES7210 hi-res RX (gated) | 🟢 | 🔴 | 🟡 | Light supervision |
| 23 | L+R sum overflow fix | 🟢 | 🟡 | 🟢 | **Hands-off** |
| 24 | Lower wet_trim to 1.0 | 🟢 | 🟡 | 🟢 | **Hands-off** (after #18) |
| 25 | Lookahead limiter | 🟡 | 🔴 | 🟡 | Light supervision |
| 26 | Anti-denormal injection | 🟡 | 🟡 | 🟢 | Light supervision |
| 27 | BPM control via encoder | 🟢 | 🟡 | 🟢 | **Hands-off** (after #1, #3) |
| 28 | BREAK stage + `SampleSource` | 🔴 | 🔴 | 🟡 | Active supervision |
| 29 | Auto-save captures to SD | 🟢 | 🟡 | 🟢 | **Hands-off** (after #2) |
| 30 | Project save/load | 🟡 | 🟡 | 🟡 | Light supervision |
| 31 | Multitrack stem export | 🔴 | 🔴 | 🔴 | Active supervision |
| 32 | Web parity tracker | 🔴 | 🔴 | 🟢 | Active supervision (ongoing) |
| 33 | Mixer overlay (deferred) | 🟡 | 🟡 | 🟡 | Light supervision (when needed) |
| 34 | SCENE stage | 🔴 | 🔴 | 🔴 | Active supervision |
| 35 | Touch alignment / tap reliability fix | 🟡 | 🟡 | 🟡 | Light supervision |
| 36 | Capture-window dead-front UI | 🟡 | 🟡 | 🟢 | Light supervision (paired with #2) |
| 37 | Euclidean chop dot ring widget | 🟡 | 🟡 | 🟢 | Light supervision |

---

## Dependency chain

```
                              FOUNDATION (do early)
                              ────────────────────
       ┌──────────── #18 ───────► #19 (ramp pairs with cap)
       │                  ─────► #24 (wet_trim, after cap)
       │
       │   #20 ──┐
       │   #23 ──┤
       │   #25 ──┼──── INDEPENDENT AUDIO QUALITY ─────►
       │   #26 ──┤
       │   #22 ──┤      (gated, A/B safe)
       │   #21 ──┘      (Faust regen — pair with #16)
       │
       │   #16 (32k SR) ◄── audit constants downstream of every issue
       │
       │   #3 (semantic input events) ────► unblocks every UI issue
       │
       │   #1 (page nav) ────► unblocks #4, #5, #8, #9, #10, #12-14, #27
       │
       ▼
                              CAPTURE FLOW
                              ────────────
   #2 (universal capture + ribbon)
       │   │
       │   ├──► #29 (auto-save WAV)
       │   │       │
       │   │       └──► #30 (project save/load)
       │   │              │
       │   │              ├──► #34 (SCENE — needs project state schema)
       │   │              └──► #32 (web parity reads/writes same JSON)
       │   │
       │   └──► #31 (stem export — also needs audio engine refactor)
       │
       ├──► #4  (gesture capture, in CHOP)
       └──► #14 (vinyl scrubber, on CAPTURE p2)

                              CHOP VARIATION (mostly parallel, low coupling)
                              ─────────────────────────────────────────────
   #5 (swing) ┐
   #6 (poly)  │── all independent, all simple, all on CHOP p2 cluster
   #7 (mut)   │
   #8 (prob)  ┘
   #10 (granular)  ──► sits alongside Euclidean as alt mode
   #11 (dual voice) ──► uses #28's SampleSource for dual-source variant later
   #12 (touch stutter) ──► hooks into CHOP touch path

                              RESONATOR / MODULATION
                              ──────────────────────
   #9 (modulation matrix) ──► RESONATE p3
   #13 (XY morph)         ──► RESONATE p2

                              NEW STAGES
                              ──────────
   #28 (BREAK)
       │ (SampleSource abstraction)
       └──► future DRUMS (uses same abstraction per design notes)
   
   #34 (SCENE)
       └── needs #30 + every engine's set_state()

                              MISC
                              ────
   #15 (mic reactive) — independent, depends on transient detector existing
   #27 (BPM control)  — depends on #1, #3
   #33 (mixer)        — deferred
   #17 (web config)   — independent backend; spawns sub-issues for many features
   #32 (web parity)   — ongoing tracker
```

---

## Recommended execution order

This is a phased plan that maximizes early audio-quality wins, lays UX foundations correctly, and defers the riskiest architectural changes until the device is otherwise solid.

### Phase 1 — Audio quality (1–2 weeks, mostly hands-off)
Surgical fixes from the punch list. Land the order matters items first.

1. **#18** — DAC volume cap (start here)
2. **#19** — DAC ramp (pairs with #18)
3. **#24** — wet_trim back to 1.0 (after #18)
4. **#23** — sum overflow fix
5. **#20** — 100 Hz HPF
6. **#22** — ES7210 hi-res RX (gated, A/B)
7. **#26** — anti-denormal
8. **#25** — lookahead limiter (more careful)
9. **#21** — 4 kHz LPF + soft-clip (subjective; better paired with #16)

After phase 1: device sounds dramatically better. Foundation for everything sonic.

### Phase 2 — UX foundation (1–2 weeks, supervised)
Unblock the rest of the roadmap. These are foundational; later issues assume them.

9.5. **#35** — touch alignment / tap reliability fix (do **before** any new touch UI)
10. **#3** — semantic input events (refactor first, low-friction after)
11. **#1** — page-based stage navigation
12. **#2** — universal capture + adjust ribbon (largest UX commit)
12.5. **#36** + **#37** — capture-ribbon and chop-ring widgets (paired with #2 / land just after)

### Phase 3 — Cheap variation wins (~1 week, hands-off)
Once foundation is in place, the chop engine variation features are mostly trivial.

13. **#27** — BPM control
14. **#5** — swing
15. **#6** — polymeter
16. **#8** — step probability
17. **#7** — Reichian mutation

After phase 3: chop output stops feeling deterministic. Big perceptual quality gain for low cost.

### Phase 4 — Sample rate bump (1 week, supervised)
Best done after audio quality fixes (validate against improved 16k baseline) and after foundation (all SR-dependent constants are visible).

18. **#16** — 32 kHz sample rate

### Phase 5 — Persistence (1–2 weeks, mixed)
Now that audio + UX foundations are solid and quality is high, persist what users make.

19. **#29** — auto-save captures
20. **#30** — project save/load
21. **#4** — gesture capture (CHOP)

### Phase 6 — Touch surfaces (2–3 weeks, supervised)
Make the dead-front actually do something on every page.

22. **#13** — RESONATE p2 morph pad
23. **#12** — CHOP press-to-stutter
24. **#14** — CAPTURE p2 vinyl scrubber (largest)

### Phase 7 — Modulation + variation depth (2–3 weeks, supervised)
Add motion to the captured material.

25. **#9** — modulation matrix + RESONATE p3
26. **#10** — granular mode
27. **#15** — mic-driven reactive

### Phase 8 — Web config foundation (2–4 weeks, supervised)
Unblocks remote setting tweaks; massive UX upgrade.

28. **#17** — BLE GATT + initial web app
29. Begin **#32** — web app parity sub-issues track in parallel from here

### Phase 9 — New stages (4–6 weeks, supervised)
The big additive features.

30. **#28** — BREAK stage (SampleSource abstraction)
31. **#11** — dual chop voices (uses #28's abstraction)
32. **#34** — SCENE stage

### Phase 10 — Production export + advanced (deferred)
Tackle once everything else is stable.

33. **#31** — multitrack stem export
34. **#33** — mixer overlay (when 2+ engines actively play together)

---

## Watch-outs not yet captured in individual issues

- **Schema versioning discipline.** #30 sets the project.json schema. Once stems (#31), scenes (#34), and web parity (#32) all read/write it, breaking changes become very expensive. Bake versioning in from day one and write a schema migration test as part of #30.
- **CPU budget at 32 kHz.** The full feature stack (resonator + dual chop + granular + modulation + drums + synth + stems) is theoretically ~85% of one core. Track this empirically as features land; if it tightens, the SR/2 internal synth path or per-engine voice limits should be discussed.
- **PSRAM allocation strategy.** Stem retention (#31) and existing retroactive buffers compete for the same PSRAM. Decide opt-in policy when #31 is implemented; document allocation budget in code.
- **MCLK lock validation in #16.** A silent failure mode is "codec didn't actually lock at 32k but didn't fail boot." Add a post-init register-readback check.
- **PA bleed in #15.** Mic-driven reactivity will feedback-loop without protection. Don't ship default-on without bench-tested PA suppression.

---

## TL;DR

- **8 issues are hands-off agent-suitable.** Mostly the audio-quality fixes (#18, #19, #20, #23, #24) and the cheapest chop variations (#5, #6, #8). #29 and #27 join once #1, #2, #3 are in place.
- **17 issues need light supervision** — agent implements, human listens before merge. Includes the new touch fix (#35), capture-ribbon widget (#36), and chop-ring widget (#37).
- **11 issues need active supervision.** These are the big architectural commits (#2, #16, #28, #31, #34, #14, #9, #17, #32) plus a few subjective sonic ones (#12).
- **Recommended start:** Phase 1 (audio quality) followed by **#35 touch fix** then Phase 2 (UX foundation). Phase 3 is mostly free quick wins on top of phase 2. Don't build new touch surfaces (#12, #13, #14) until #35 is verified-fixed — the touch reliability problem will mask all of them.
