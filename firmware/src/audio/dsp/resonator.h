/**
 * Resonator DSP Module
 *
 * Wraps the Faust-compiled MorphingResonatorESP32 (5 voices x 3 harmonics + Freeverb).
 * Internal processing is float32; the wrapper handles int16 conversion.
 */

#ifndef DSP_RESONATOR_H
#define DSP_RESONATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Resonator_init(void);

/**
 * Process audio in-place style: read from input (int16 mono), write to output (int16 mono).
 * Safe to call with input == output.
 */
void Resonator_process(const int16_t* input, int16_t* output, uint32_t samples);

/** 0..15 — chord index into the resonator's chord dictionary */
void Resonator_setChord(uint8_t chordIdx);

/** 24..72 (MIDI) — root note for the chord */
void Resonator_setRootNote(uint8_t midiNote);

/** 0..1 — dry/wet mix (0 = pure dry, 1 = pure wet) */
void Resonator_setWetDry(float mix);

/** 0..2 — timbre morph (0=harmonic, 1=inharmonic, 2=metallic/fixed) */
void Resonator_setTimbre(float t);

/** 0.1..15 seconds — resonator decay */
void Resonator_setDecay(float seconds);

/** 0..4 — voice emphasis index */
void Resonator_setVoicing(uint8_t v);

/** Legacy API kept for compile-compat with audio_engine; map onto useful params. */
void Resonator_setFilterCutoff(float cutoffHz);
void Resonator_setResonance(float q);
void Resonator_setScale(uint8_t scaleType, uint8_t rootNote);

#ifdef __cplusplus
}
#endif

#endif // DSP_RESONATOR_H
