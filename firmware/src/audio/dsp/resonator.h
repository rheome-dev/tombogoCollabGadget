/**
 * Resonator DSP Module
 *
 * Scale-locked resonator that creates harmonic content from mic input.
 * Written in Faust for portability.
 */

#ifndef DSP_RESONATOR_H
#define DSP_RESONATOR_H

#include <stdint.h>

/**
 * Resonator modes
 */
enum ResonatorMode {
    RESONATOR_STRING,   // String-like harmonics
    RESONATOR_VOCAL,    // Vocal formant-like
    RESONATOR_MODAL,    // Modal synthesis
    RESONATOR_BELL      // Bell-like
};

/**
 * Initialize resonator
 */
void Resonator_init(void);

/**
 * Process audio
 * @param input Input samples
 * @param output Output samples
 * @param samples Sample count
 */
void Resonator_process(const int16_t* input, int16_t* output, uint32_t samples);

/**
 * Set filter cutoff frequency
 */
void Resonator_setFilterCutoff(float cutoffHz);

/**
 * Set resonance/Q
 */
void Resonator_setResonance(float q);

/**
 * Set decay time
 */
void Resonator_setDecay(float seconds);

/**
 * Set scale for pitch locking
 */
void Resonator_setScale(uint8_t scaleType, uint8_t rootNote);

/**
 * Set resonator mode
 */
void Resonator_setMode(ResonatorMode mode);

#endif // DSP_RESONATOR_H
