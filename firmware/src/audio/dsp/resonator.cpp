/**
 * Resonator DSP Module — Faust wrapper
 *
 * Allocates the FaustResonator state in PSRAM (state can be ~50KB+ from
 * mode filters + Freeverb delay lines), keeps two AUDIO_BUFFER_SIZE float
 * scratch buffers in DRAM for the int16↔float conversion, and exposes a
 * small C API used by audio_engine.
 */

#include "resonator.h"
#include "../../config.h"
#include "faust_resonator.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <new>     // placement new
#include <string.h>

// ─── State ──────────────────────────────────────────────────────────────────

static FaustResonator* g_dsp = nullptr;

// Scratch float buffers — DRAM (latency-sensitive path, 256 floats each = 1KB).
static float g_inBuf[AUDIO_BUFFER_SIZE];
static float g_outBuf[AUDIO_BUFFER_SIZE];

// ─── Init ───────────────────────────────────────────────────────────────────

void Resonator_init(void) {
    if (g_dsp) return;

    // Allocate the DSP state in PSRAM. The struct is large (mode filters +
    // Freeverb delay lines), so it would be wasteful in DRAM. PSRAM access
    // is slower but the per-sample work dominates anyway.
    void* mem = heap_caps_malloc(sizeof(FaustResonator), MALLOC_CAP_SPIRAM);
    if (!mem) {
        Serial.printf("Resonator: PSRAM alloc failed for %u bytes — falling back to DRAM\n",
                      (unsigned)sizeof(FaustResonator));
        mem = malloc(sizeof(FaustResonator));
    }
    if (!mem) {
        Serial.println("Resonator: ALLOC FAILED — resonator disabled");
        return;
    }

    g_dsp = new (mem) FaustResonator();
    g_dsp->init(SAMPLE_RATE);

    // Slider mapping (after Faust regen with reverb_mix added):
    //   fHslider0 = Reverb Mix      fHslider5 = Drive
    //   fHslider1 = Output Gain     fHslider6 = Decay
    //   fHslider2 = Dry/Wet Mix     fHslider7 = Root Note
    //   fHslider3 = Resonator Trim  fHslider8 = Chord Glide
    //   fHslider4 = Input Trim      fHslider9 = Timbre
    //   fEntry0 = Voicing Index     fEntry1 = Chord Grid
    //
    // Gain staging: the looper feeds at near full-scale, mode filters amplify
    // resonant frequencies 10–100×. Saturation has been removed in the .dsp
    // (drive is now pure linear gain), so input_trim is the primary headroom
    // knob — keep it low to prevent the final hard [-1, 1] clamp from clipping.
    g_dsp->fHslider4 = 0.10f;   // Input Trim
    g_dsp->fHslider5 = 1.0f;    // Drive (no saturation, just linear)
    g_dsp->fHslider3 = 0.4f;    // Resonator Trim
    g_dsp->fHslider2 = 0.0f;    // Dry/Wet — start dry, audio_engine ramps it
    g_dsp->fHslider1 = 1.0f;    // Output Gain
    g_dsp->fHslider0 = 0.0f;    // Reverb Mix — OFF by default

    Serial.printf("Resonator: initialized (FaustResonator state %u bytes, %s)\n",
                  (unsigned)sizeof(FaustResonator),
                  heap_caps_get_allocated_size(mem) > 0 ? "PSRAM" : "DRAM");
}

// ─── Process ────────────────────────────────────────────────────────────────

void Resonator_process(const int16_t* input, int16_t* output, uint32_t samples) {
    if (!g_dsp || !input || !output || samples == 0) {
        if (input && output && input != output) {
            memcpy(output, input, samples * sizeof(int16_t));
        }
        return;
    }

    // Faust compute() works in chunks ≤ AUDIO_BUFFER_SIZE. The audio engine
    // calls us with monoCount == AUDIO_BUFFER_SIZE so this loop usually runs
    // once, but stay defensive for partial frames.
    uint32_t remaining = samples;
    const int16_t* in = input;
    int16_t* out = output;

    while (remaining > 0) {
        uint32_t chunk = remaining > AUDIO_BUFFER_SIZE ? AUDIO_BUFFER_SIZE : remaining;

        // int16 → float [-1, 1)
        for (uint32_t i = 0; i < chunk; i++) {
            g_inBuf[i] = (float)in[i] * (1.0f / 32768.0f);
        }

        float* inputs[1]  = { g_inBuf };
        float* outputs[1] = { g_outBuf };
        g_dsp->compute((int)chunk, inputs, outputs);

        // float → int16 with hard clamp
        for (uint32_t i = 0; i < chunk; i++) {
            float s = g_outBuf[i] * 32767.0f;
            if (s > 32767.0f)  s = 32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            out[i] = (int16_t)s;
        }

        in        += chunk;
        out       += chunk;
        remaining -= chunk;
    }
}

// ─── Parameter Setters ──────────────────────────────────────────────────────
//
// Faust generates per-frame slewing for slider params (fSlow* in compute),
// so direct writes are safe — the DSP smooths internally via si.smoo on
// the relevant params.

void Resonator_setChord(uint8_t chordIdx) {
    if (!g_dsp) return;
    if (chordIdx > 15) chordIdx = 15;
    g_dsp->fEntry1 = (FAUSTFLOAT)chordIdx;
}

void Resonator_setRootNote(uint8_t midiNote) {
    if (!g_dsp) return;
    if (midiNote < 24) midiNote = 24;
    if (midiNote > 72) midiNote = 72;
    g_dsp->fHslider7 = (FAUSTFLOAT)midiNote;
}

void Resonator_setWetDry(float mix) {
    if (!g_dsp) return;
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    g_dsp->fHslider2 = (FAUSTFLOAT)mix;
}

void Resonator_setTimbre(float t) {
    if (!g_dsp) return;
    if (t < 0.0f) t = 0.0f;
    if (t > 2.0f) t = 2.0f;
    g_dsp->fHslider9 = (FAUSTFLOAT)t;
}

void Resonator_setDecay(float seconds) {
    if (!g_dsp) return;
    if (seconds < 0.1f)  seconds = 0.1f;
    if (seconds > 15.0f) seconds = 15.0f;
    g_dsp->fHslider6 = (FAUSTFLOAT)seconds;
}

void Resonator_setVoicing(uint8_t v) {
    if (!g_dsp) return;
    if (v > 4) v = 4;
    g_dsp->fEntry0 = (FAUSTFLOAT)v;
}

// ─── Legacy compatibility shims ─────────────────────────────────────────────
// audio_engine.cpp still calls these. Map them onto useful parameters so the
// IMU and scale wiring still produces audible changes.

void Resonator_setFilterCutoff(float cutoffHz) {
    // No direct cutoff in this voice-based resonator. Map IMU-derived cutoff
    // (~80 Hz to ~8 kHz) onto root-note pitch (C2..C5 range = MIDI 36..72).
    if (cutoffHz < 80.0f)   cutoffHz = 80.0f;
    if (cutoffHz > 8000.0f) cutoffHz = 8000.0f;
    float norm = (cutoffHz - 80.0f) / (8000.0f - 80.0f);   // 0..1
    uint8_t midi = 36 + (uint8_t)(norm * 36.0f);
    Resonator_setRootNote(midi);
}

void Resonator_setResonance(float q) {
    // Map Q (0..1+) onto decay (0.5..8s)
    if (q < 0.0f) q = 0.0f;
    if (q > 1.0f) q = 1.0f;
    Resonator_setDecay(0.5f + q * 7.5f);
}

void Resonator_setScale(uint8_t scaleType, uint8_t rootNote) {
    Resonator_setChord(scaleType & 0x0F);
    Resonator_setRootNote(rootNote);
}
