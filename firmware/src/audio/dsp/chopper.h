/**
 * Rhythmic Chopper DSP Module
 *
 * XLN Audio Life-style algorithmic sample chopping.
 * Written in Faust for portability.
 */

#ifndef DSP_CHOPPER_H
#define DSP_CHOPPER_H

#include <stdint.h>

/**
 * Chop patterns
 */
enum ChopPattern {
    CHOP_STRAIGHT,
    CHOP_SWUNG,
    CHOP_RANDOM,
    CHOP_SHUFFLE
};

/**
 * Initialize chopper
 */
void Chopper_init(void);

/**
 * Process audio
 */
void Chopper_process(const int16_t* input, int16_t* output, uint32_t samples);

/**
 * Set tempo
 */
void Chopper_setTempo(uint16_t bpm);

/**
 * Set chop density (0-1)
 */
void Chopper_setDensity(float density);

/**
 * Set randomness (0-1)
 */
void Chopper_setRandomness(float random);

/**
 * Set pattern type
 */
void Chopper_setPattern(ChopPattern pattern);

/**
 * Trigger new chop sequence
 */
void Chopper_trigger(void);

#endif // DSP_CHOPPER_H
