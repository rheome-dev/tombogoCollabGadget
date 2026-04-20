/**
 * Simple Synth DSP Module
 *
 * Basic synthesizer not requiring microphone input.
 * Written in Faust for portability.
 */

#ifndef DSP_SYNTH_H
#define DSP_SYNTH_H

#include <stdint.h>

/**
 * Waveform types
 */
enum Waveform {
    WAVE_SINE,
    WAVE_SAW,
    WAVE_SQUARE,
    WAVE_TRIANGLE
};

/**
 * Initialize synth
 */
void Synth_init(void);

/**
 * Process audio (generates output)
 */
void Synth_process(int16_t* output, uint32_t samples);

/**
 * Trigger a note
 * @param note MIDI note number (0-127)
 * @param velocity 0-127
 */
void Synth_noteOn(uint8_t note, uint8_t velocity);

/**
 * Release note
 */
void Synth_noteOff(void);

/**
 * Set waveform
 */
void Synth_setWaveform(Waveform wave);

/**
 * Set ADSR envelope
 */
void Synth_setEnvelope(float attack, float decay, float sustain, float release);

/**
 * Set pitch bend
 */
void Synth_setPitchBend(float semitones);

/**
 * Set scale for note locking
 */
void Synth_setScale(uint8_t scaleType, uint8_t rootNote);

/**
 * Get current frequency
 */
float Synth_getFrequency(void);

#endif // DSP_SYNTH_H
