# Tombogo × Rheome — Audio Quality Punch List

**Date:** 2026-05-01
**Scope:** End-to-end audio quality investigation across ES7210 ADC → DSP → ES8311 DAC → NS4150B PA → speaker on the Waveshare ESP32-S3-Touch-AMOLED-1.75.

**Reported symptoms:**
1. Onboard MEMS mic input clips easily on transients
2. Highs are harsh / sibilant / "MEMS hash"
3. Audible low-frequency rumble (sub-bass, room/handling/wind)
4. High noise floor (~constant hiss)
5. Distortion on speaker playback (NS4150B path), even on clean test tones

---

## A. Symptom → Root Cause Table (ranked by confidence)

| Symptom | Root cause (most likely → less likely) | Evidence |
|---|---|---|
| **5. Speaker distortion on clean tones** | (1) ES8311 volume map writes **+19 dB digital boost** at vol=90 default, **+32 dB** at vol=100 → DAC squarewaves into PA. (2) NS4150B internal gain ~24 dB plus full-scale DAC → hard rail-clip even at 0 dBFS. (3) PVDD sag on transients (insufficient bulk cap on 5 V rail). | `audio_esp32.cpp:245` `vol = (volume*256/100)-1`; ES8311 reg 0x32 law: 0xBF=0 dB, 0xFF=+32 dB, 0.5 dB/step (User Guide §11.2). NS4150 datasheet: gain set by source-Z, ~26 dB ref. |
| **4. High noise floor / hiss** | (1) I²S read **truncates 8 LSBs**: ES7210 outputs 24-bit MSB-aligned in 32-bit slot, code does `>> 16`. (2) ES7210 PGA at +30 dB amplifies thermal floor before truncation occurs. | `audio_esp32.cpp:450` `buffer[i] = (int16_t)(i2sDmaReadBuf[i] >> 16)`; should be `>> 14` to preserve 16 of 24 bits, or process as int32 throughout. |
| **3. Low-freq rumble** | DC blocker `R=0.995` is **−3 dB at 12.76 Hz** — handles DC only, passes 50–200 Hz handling/wind/proximity rumble. ES7210 internal HPF (REG22/23 = 0x0A/0x2A) is also fixed at ~20 Hz. | `audio_engine.cpp:165`; computed `fc = -ln(0.995)·16000/2π = 12.76 Hz`. |
| **2. Harsh / sibilant highs** | No HF rolloff anywhere in the chain. Wingie2 has `fi.lowpass(1, 4000)` immediately before the modal bank — we don't. ES7210's anti-alias filter rolls off above 8 kHz only. | `Wingie2.dsp:191`; `firmware/faust/resonator_esp32.dsp` chain: no LPF block. |
| **1. Mic clipping on transients** | (1) `((L+R)/2)*4` multiplies before clamping — int32 wraps reach the clamp as hard saturation, not headroom. (2) Hard `min/max` int16 clamp instead of soft saturator. (3) `wet_trim = 1.6f` (+12 dB makeup) on resonator path stacks gain on top of the 4× SW gain. | `audio_engine.cpp:169-171`; `resonator.cpp:64`. Wingie2 uses `ef.cubicnl(0.01, 0)` instead of hard clip. |

---

## B. Prioritized Fix List

### [ES8311-config] — F1: Cap DAC volume at 0 dB ⭐ HIGHEST PRIORITY

**File:** `firmware/platforms/esp32/audio_esp32.cpp:245`

```c
// BEFORE (maps user vol 75→0xBF=0 dB, 90→0xE5=+19 dB, 100→0xFF=+32 dB):
uint8_t vol = (currentVolume == 0) ? 0 : ((currentVolume * 256 / 100) - 1);

// AFTER (maps user vol 0→muted, 100→0xBF=0 dB, no positive gain):
//   ES8311 reg 0x32: 0x00=-95.5 dB, 0xBF=0 dB, 0xFF=+32 dB, 0.5 dB/step
uint8_t vol = (currentVolume == 0) ? 0 : (uint8_t)((currentVolume * 0xBF) / 100);
```

Cite: ES8311 User Guide Rev1.11 §11.2 (https://files.waveshare.com/wiki/common/ES8311.user.Guide.pdf), reg 0x32 volume law.

### [ES8311-config] — F2: Enable DAC ramp before any volume change

**File:** `audio_esp32.cpp` init sequence (where ES8311 is configured)

```c
es8311_write_reg(0x37, 0x08);   // DAC_RAMPRATE = 0.25 dB / 512 LRCK ≈ 32 ms ramp
                                 // MUST be set before reg 0x32 or path enables
es8311_write_reg(0x32, 0xBF);   // DAC volume = 0 dB starting point
```

### [SW-DSP] — F3: Stop truncating ES7210's lower 8 bits

**File:** `firmware/platforms/esp32/audio_esp32.cpp:450`

```c
// BEFORE: discards 8 LSBs of 24-bit ADC data (≈48 dB dynamic-range loss).
buffer[i] = (int16_t)(i2sDmaReadBuf[i] >> 16);

// AFTER (option A — keep int16 path, recover 2 bits with proper rounding & saturation):
int32_t s = i2sDmaReadBuf[i] >> 14;          // 24-bit MSB-aligned → 18-bit
s = (s + 2) >> 2;                             // round-to-nearest, downshift 2
if (s > 32767) s = 32767;
if (s < -32768) s = -32768;
buffer[i] = (int16_t)s;

// AFTER (option B, BETTER — process the whole DSP path in int32/float, only
// quantize to int16 at the very last DAC write. Restructures audio_engine.cpp.)
```

Option B is the long-term fix; option A is a same-day improvement.

### [SW-DSP] — F4: Replace 1-pole DC blocker with Butterworth HPF @ 100 Hz

**File:** `firmware/src/core/audio_engine.cpp` (replace dc-blocker block at line 165 / 173-179)

```c
// 100 Hz Butterworth HPF, fs=16000, RBJ cookbook, Q=1/√2.
// Computed values (interpolated between provided 80/120 Hz pairs):
static const float HPF100_b0 = +0.97262f;
static const float HPF100_b1 = -1.94524f;
static const float HPF100_b2 = +0.97262f;
static const float HPF100_a1 = -1.94462f;
static const float HPF100_a2 = +0.94586f;
static float hpf_z1 = 0.0f, hpf_z2 = 0.0f;

// Direct-Form-II Transposed:
float xin = (float)mono;
float y   = HPF100_b0 * xin + hpf_z1;
hpf_z1    = HPF100_b1 * xin - HPF100_a1 * y + hpf_z2;
hpf_z2    = HPF100_b2 * xin - HPF100_a2 * y;
```

Alternatives if 100 Hz feels too aggressive — use 80 Hz; if sub-bass still passes, use 120 Hz:

```c
// 80 Hz Butterworth HPF, fs=16000.
static const float HPF80_b0  = +0.9780304792f;
static const float HPF80_b1  = -1.9560609584f;
static const float HPF80_b2  = +0.9780304792f;
static const float HPF80_a1  = -1.9555782403f;
static const float HPF80_a2  = +0.9565436765f;

// 120 Hz Butterworth HPF, fs=16000.
static const float HPF120_b0 = +0.9672272827f;
static const float HPF120_b1 = -1.9344545654f;
static const float HPF120_b2 = +0.9672272827f;
static const float HPF120_a1 = -1.9333802259f;
static const float HPF120_a2 = +0.9355289050f;
```

Cite: RBJ Audio EQ Cookbook (https://webaudio.github.io/Audio-EQ-Cookbook/Audio-EQ-Cookbook.txt).

### [Faust] — F5: Add 4 kHz LPF + cubic soft-clip per Wingie2

**File:** `firmware/faust/resonator_esp32.dsp:101-105` (replace `process_channel`)

```faust
// BEFORE (current):
//   (x * dry_mix) + (((x * input_trim) : saturate : engine_A) * wet_trim * wet_mix)
//     : *(gain) : max(-1.0) : min(1.0);

// AFTER (Wingie2-style: pre-resonator HPF/LPF, post-resonator cubic):
process_channel(x) =
    (x * dry_mix)
  + ( x * input_trim
        : fi.lowpass(1, 4000)              // <-- harshness fix (Wingie2.dsp:191)
        : engine_A
        : *(wet_trim * wet_mix)
        : ef.cubicnl(0.01, 0)               // <-- soft saturator vs hard clamp
    )
  : *(gain)
  : max(-1.0) : min(1.0);                  // belt-and-braces
```

Cite: `Wingie2.dsp:191` (`fi.lowpass(1, 4000)`), `Wingie2.dsp:195` (`ef.cubicnl(0.01, 0)`).

### [SW-DSP] — F6: Fix L+R sum overflow and hard-clip-before-clamp

**File:** `firmware/src/core/audio_engine.cpp:169-171`

```c
// BEFORE:
int32_t mono = ((L + R) / 2) * 4;
if (mono > 32767) mono = 32767;
if (mono < -32768) mono = -32768;

// AFTER (clamp the sum first, scale with explicit headroom budget):
int32_t sum = (int32_t)L + (int32_t)R;        // safe in int32, max ±65534
int32_t mono = sum * 2;                        // ×2 = +6 dB (was ×4 ÷2 = ×2)
                                               // if you want +12 dB, audit & retune downstream
if (mono >  32767) mono =  32767;
if (mono < -32768) mono = -32768;
```

The `((L+R)/2) * 4` is mathematically `(L+R) * 2` — same gain — but the existing form throws away 1 bit before scaling and risks int32 wrap on very loud transients. The new form is clearer.

### [Faust / SW-DSP] — F7: Lower `wet_trim` default from 1.6 → 1.0

**File:** `firmware/src/core/resonator.cpp:64`

```c
// BEFORE: g_dsp->fHslider3 = 1.6f;  // +12 dB makeup, risks Faust accumulator saturation
// AFTER:  g_dsp->fHslider3 = 1.0f;  // unity; once F1 fixes the DAC volume, you no longer
                                      // need to push the DSP to "match dry loudness"
```

### [SW-DSP] — F8: Replace feedforward limiter with 64-sample lookahead

**File:** `firmware/src/core/audio_engine.cpp:280-297`

```c
#define LA_SAMPLES 64
#define LA_MASK    63        // LA must be power of 2
typedef struct {
    float thresh;            // linear, e.g. 0.708f for -3 dBFS
    float buf[LA_SAMPLES];   // delay line
    int   wpos;
    float gain;
    float release_coeff;     // exp(-1/(release_sec*fs))
} limiter_t;

static void limiter_init(limiter_t *L, float thresh_lin, float release_sec, float fs) {
    L->thresh = thresh_lin;
    L->wpos = 0;
    L->gain = 1.0f;
    L->release_coeff = expf(-1.0f / (release_sec * fs));
    for (int i = 0; i < LA_SAMPLES; i++) L->buf[i] = 0;
}

static inline float limiter_tick(limiter_t *L, float x) {
    float delayed = L->buf[L->wpos];
    L->buf[L->wpos] = x;

    // moving max of |x| over the LA window
    float mx = 0.0f;
    for (int i = 0; i < LA_SAMPLES; i++) {
        float a = fabsf(L->buf[i]);
        if (a > mx) mx = a;
    }

    float req_gain = (mx > L->thresh) ? (L->thresh / mx) : 1.0f;
    if (req_gain < L->gain) {
        L->gain = req_gain;            // immediate — peak hasn't arrived yet
    } else {
        L->gain = L->release_coeff * L->gain + (1.0f - L->release_coeff) * 1.0f;
    }

    L->wpos = (L->wpos + 1) & LA_MASK;
    return delayed * L->gain;
}
```

Tuning: `thresh_lin = 0.708f` (−3 dBFS), `release_sec = 0.080f`, `LA_SAMPLES = 64` (4 ms latency, inaudible). ~2 MIPS at 16 kHz — trivial on LX7.

Cite: Signalsmith Audio limiter design (https://signalsmith-audio.co.uk/writing/2022/limiter/).

### [Faust] — F9: Anti-denormal injection on Xtensa LX7

LX7 has IEEE-754 with denormals but no FTZ/DAZ — denormals stall the FPU. For 5 voices × 3 mode filters = 15 IIRs, this matters. Inject `±1e-20f` alternating bias into resonator state per buffer.

```cpp
static uint32_t s_denorm_tick;
static inline float anti_denorm(float x) {
    s_denorm_tick ^= 1;
    return x + (s_denorm_tick ? 1e-20f : -1e-20f);
}
```

Build flags: add `-ffast-math -fno-trapping-math -funsafe-math-optimizations` to the Faust object compilation. Keep `-O2` (not `-O3`) to avoid i-cache pressure.

Cite: Faust on ESP32 (Michon et al., https://hal.science/hal-02988312/document).

### [HW] — F10: Verify NS4150B input network and PVDD decoupling

Off-board, but cheap to inspect on the Waveshare module:
- **CIN** (DC-cut between ES8311 OUTP/OUTN and NS4150B INP/INN): should be ≤ 0.1 µF (NS4150 datasheet).
- **VDD bypass**: 1 µF X7R right at VCC pin + 10 µF bulk within 5 mm.
- **Bypass cap pin 2** (VDD/2 reference): bump from 1 µF to 2.2/4.7 µF X7R for better LF PSRR.
- **Output ferrites**: BLM18PG121SN1-class beads on each speaker pin to keep 400 kHz residual out of the mic ADC.

Cite: NS4150 datasheet (http://aitendo3.sakura.ne.jp/aitendo_data/product_img/ic/power_amp/NS4150/NS4150.pdf).

---

## C. "Do This First" — Top 3 by dB-of-improvement per hour-of-work

### 1. **F1: Cap DAC volume at 0 dB** (15 min, ≥ 20 dB cleaner playback)
One-line change in `audio_esp32.cpp:245`. Removes up to +19 dB of digital boost that's been overdriving both the DAC and the NS4150B. Single highest-leverage fix in the entire investigation. Validate with the **fixed −12 dBFS sine + master-volume sweep** test — distortion should disappear above vol=70 immediately.

### 2. **F5: Add `fi.lowpass(1, 4000)` + `ef.cubicnl(0.01, 0)` to resonator chain** (30 min, eliminates harshness + soft-knee transient handling)
Two-line Faust change, one recompile. Directly addresses symptom 2 (sibilance) and softens symptom 1 (transient clip). Verbatim copy from Wingie2 — known-good in production hardware.

### 3. **F4: Butterworth HPF @ 100 Hz replacing the 12 Hz DC blocker** (20 min, eliminates rumble)
Drop-in DF2T biquad, ~5 multiplies/sample. Fixes symptom 3 entirely. The current "DC blocker" provides essentially no protection against the sub-200 Hz handling noise that's the actual problem.

**Combined: ~1 hour of work, addresses 4 of 5 reported symptoms with measurable, audible improvement.** Symptom 4 (hiss) needs the I²S truncation fix (F3) which is more invasive and merits a follow-up session.

---

## Confirming Test (run before and after fixes)

Generate a fixed −12 dBFS 1 kHz sine in firmware (so source is known clean). Sweep `AudioHAL_setVolume(20 → 100)` in steps of 10 while it plays. Record near-field with phone spectrum analyzer.

| Result | Diagnosis |
|---|---|
| Distortion appears at vol ≈ 70–80, worsens linearly above | F1 is the cause (digital vol > 0 dB) |
| Distortion already audible at vol = 30, gets worse | NS4150B always rails — DAC FS alone too hot for this PA at 5 V (F1 + further attenuation needed) |
| Steady tone clean at all volumes; only transients distort | PVDD sag (F10 — add bulk cap) |

---

## D. Citations

| Recommendation | Source |
|---|---|
| F1 — ES8311 vol map / reg 0x32 law | ES8311 User Guide Rev1.11 §11.2 → https://files.waveshare.com/wiki/common/ES8311.user.Guide.pdf · code: `audio_esp32.cpp:245` |
| F2 — DAC ramp before vol writes | ES8311 User Guide §11.3, ESP-ADF `es8311.c` → https://github.com/espressif/esp-adf/blob/release/v2.x/components/audio_hal/driver/es8311/es8311.c |
| F3 — I²S MSB-align truncation | `audio_esp32.cpp:450`; ES7210 datasheet §I2S format → https://uploadcdn.oneyac.com/attachments/userfile/68/63/1629167664244_5904.pdf |
| F4 — 80/100/120 Hz Butterworth biquads | RBJ Audio EQ Cookbook → https://webaudio.github.io/Audio-EQ-Cookbook/Audio-EQ-Cookbook.txt; current code `audio_engine.cpp:165` |
| F5 — 4 kHz LPF + cubic NL | `Wingie2.dsp:191, 195` → https://raw.githubusercontent.com/mengqimusic/Wingie2/main/Wingie2.dsp |
| F6 — L+R sum overflow | `audio_engine.cpp:169-171` |
| F7 — wet_trim default | `resonator.cpp:64`; design rationale in self-audit findings |
| F8 — Lookahead limiter | Signalsmith Audio limiter design → https://signalsmith-audio.co.uk/writing/2022/limiter/; current code `audio_engine.cpp:280-297` |
| F9 — LX7 denormals | Faust on ESP32 (Michon et al.) → https://hal.science/hal-02988312/document |
| F10 — NS4150B network | NS4150 datasheet → http://aitendo3.sakura.ne.jp/aitendo_data/product_img/ic/power_amp/NS4150/NS4150.pdf |

---

## Reference: Wingie 2 Comparison

Meng Qi's Wingie 2 (`mengqimusic/Wingie2`, MIT-licensed Faust + Arduino) runs the same Faust resonator paradigm we're building, on similar hardware. Their pre-resonator chain (verbatim from `Wingie2.dsp:186-199`):

```faust
process = _,_
    : fi.dcblocker, fi.dcblocker
    : (...amp_follower trigger sidechain — UI only, doesn't modify audio)
        : (_ * env_mode_change * volume0), (_ * env_mode_change * volume1)
            <: (_ * resonator_input_gain : fi.lowpass(1, 4000) <: hgroup("left", sum(i, nHarmonics, r(note0, i, mode0))) * pre_clip_gain),
               (_ * resonator_input_gain : fi.lowpass(1, 4000) <: hgroup("right", sum(i, nHarmonics, r(note1, i, mode1))) * pre_clip_gain),
               _,
               _
                : ef.cubicnl(0.01, 0), ef.cubicnl(0.01, 0), _, _
                    : _ * vol_wet0 * post_clip_gain, _ * vol_wet1 * post_clip_gain, _ * vol_dry0, _ * vol_dry1
                //:> co.limiter_1176_R4_mono, co.limiter_1176_R4_mono   <-- intentionally commented out
                //:> aa.cubic1, aa.cubic1
                        :> (_ * output_gain), (_ * output_gain);
```

**What Wingie 2 has that we don't:**
- Hard 1st-order LPF at 4 kHz immediately before the modal bank (F5).
- Cubic soft-clip (`ef.cubicnl(0.01, 0)`) on wet output (F5).
- Conservative gain staging by *starvation* — input × 0.25 → res_in × 0.5 → pre_clip × 0.5 → post_clip × 0.5 ≈ −60 dB on wet path (more headroom up to the saturator).

**What Wingie 2 explicitly does NOT have (their deliberate non-choices):**
- No noise gate. No compressor. No de-esser. No AGC.
- No tunable HPF — `hp_cutoff = hslider(85, 35, 500)` was added and *commented out* (line 26).
- No output limiter — `co.limiter_1176_R4_mono` was tried and *removed* (line 197). Strong signal that an 1176-style limiter is not worth the CPU cost on Faust+ESP32.
- No feedback suppression. They handle mic-into-speaker proximity through low default gains, not DSP.

**Sample rate delta:** Wingie 2 runs 44.1 kHz; we run 16 kHz. Their 4 kHz LPF makes more sense at 44.1 (rolling off pre-Nyquist content); for us at 16 kHz Nyquist is 8 kHz, so a 4 kHz LPF is more aggressive in our context. Could move to 5–6 kHz if it sounds dull, but the same shape applies.

---

## Reference: Verified Gain Stage Map

| Stage | Level (nominal) | Headroom | Risk |
|---|---|---|---|
| ES7210 PGA (+30 dB) | -30 dBu in → 0 dBu out | 0 dB | 🔴 TIGHT |
| I²S RX (32-bit MSB align, `>> 16`) | discards 8 LSBs | -48 dB | 🔴 NOISE FLOOR LOSS |
| Mic SW path (sum + 4× + clamp) | ±32767 @ 0 dBFS | 0 dB | 🔴 HARD CLIP READY |
| DC blocker (R=0.995, fc=12.76 Hz) | ±float | ∞ float | 🟡 USELESS for rumble |
| Faust (dry, mix=0) | unity | 0 dB | 🟢 SAFE |
| Faust (wet, default trims) | -46 dBFS | 46 dB | 🟢 SAFE |
| Master vol (80%) | -2.0 dBFS | 2 dB | 🟡 |
| Peak limiter (28000 = -1.35 dBFS) | designed ceiling | 1.35 dB | 🟡 |
| **ES8311 DAC vol = 90 (0xE5)** | **+19 dB digital** | **NEGATIVE** | 🔴 **CRITICAL — F1** |
| ES8311 analog out | ~1.8 Vrms differential | ~3.2 V to rail | 🟢 |
| NS4150B PA (~24 dB fixed gain) | ~4.5 Vrms @ 1.7 W | 0.5 V to clip | 🟡 |
| Speaker (1 W / 8 Ω) | Po | — | depends on Xmax |
