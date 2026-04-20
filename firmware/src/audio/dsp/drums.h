/**
 * Drum Machine DSP Module
 *
 * Simple drum sample player with 16-step sequencer.
 * Written in Faust for portability.
 */

#ifndef DSP_DRUMS_H
#define DSP_DRUMS_H

#include <stdint.h>

/**
 * Drum sounds
 */
enum DrumSound {
    DRUM_KICK,
    DRUM_SNARE,
    DRUM_HIHAT,
    DRUM_CLAP,
    DRUM_TOM,
    DRUM_COUNT
};

/**
 * Initialize drum machine
 */
void Drums_init(void);

/**
 * Process audio
 */
void Drums_process(int16_t* output, uint32_t samples);

/**
 * Set tempo
 */
void Drums_setTempo(uint16_t bpm);

/**
 * Set pattern for a drum
 * @param drum Which drum
 * @param pattern 16-bit pattern (bit per step)
 */
void Drums_setPattern(DrumSound drum, uint16_t pattern);

/**
 * Set volume for a drum
 */
void Drums_setVolume(DrumSound drum, float volume);

/**
 * Start sequencer
 */
void Drums_start(void);

/**
 * Stop sequencer
 */
void Drums_stop(void);

/**
 * Step sequencer (call at each step)
 */
void Drums_step(void);

#endif // DSP_DRUMS_H
