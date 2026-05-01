/**
 * Chop Engine - Euclidean rhythmic slicer
 *
 * Slices captured audio into sections and plays them back in a
 * Bjorklund Euclidean pattern. Supports 3 playback types:
 * - Normal: forward playback
 * - Reverse: backward playback
 * - Ratchet: rapid repetitions
 */

#ifndef CHOP_ENGINE_H
#define CHOP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "transient_detector.h"

// ─── Playback Types ───────────────────────────────────────────────────────────

typedef enum {
    PLAY_NORMAL = 0,
    PLAY_REVERSE,
    PLAY_RATCHET
} PlaybackType;

// ─── State ───────────────────────────────────────────────────────────────────

typedef struct {
    // Slice metadata (from transient detector or grid)
    SliceInfo     slices[MAX_SLICES];
    uint8_t       numSlices;

    // Euclidean pattern
    bool          pattern[16];      // 16-step grid
    uint8_t       playbackType[16]; // Per-step playback type
    uint8_t       steps;             // Total pattern length (steps)

    // Parameters
    float         density;           // 0.0-1.0 → 2-16 pulses
    bool          active;            // Chop enabled
    bool          mute;              // Mute output (crossfade out)

    // Runtime state
    uint8_t       currentStep;       // 0-15
    uint32_t      stepSamples;      // Samples per sixteenth note
    uint32_t      sampleCounter;    // Counts samples toward next step

    // Crossfade
    bool          fadeOut;           // True = fading out
    uint16_t      fadeSamples;       // Fade duration in samples

    // Pitch shift: source-read scaling. 1.0 = original pitch, 2.0 = octave up.
    float         pitchRate;
} ChopState;

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * Initialize the chop engine.
 */
void ChopEngine_init(void);

/**
 * Load slices from transient detection.
 */
void ChopEngine_setSlices(const SliceInfo* slices, uint8_t count);

/**
 * Set density (0.0-1.0).
 * Recalculates Euclidean pattern.
 */
void ChopEngine_setDensity(float density);

/**
 * Enable/disable chop.
 */
void ChopEngine_setActive(bool active);

/**
 * Called on each sixteenth note from BPM clock.
 * Processes and outputs slice samples.
 *
 * @param input      Input buffer (from loop playback)
 * @param output     Output buffer (chopped result)
 * @param samples    Number of mono samples to process
 * @param sliceLen   Current slice length (from BPM)
 */
void ChopEngine_process(const int16_t* input, int16_t* output,
                         uint32_t samples, uint32_t sliceLen);

/**
 * Randomize playback types for all active steps AND rotate the pattern by a
 * random offset so different slices land on different beats. Called on joystick
 * center — the rotation is what makes the change audibly obvious.
 */
void ChopEngine_randomize(void);

/**
 * Set pitch rate (1.0 = original, 2.0 = octave up, 0.5 = octave down).
 * Applied to slice source reads so chopped output tracks loop pitch shift.
 */
void ChopEngine_setPitchRate(float rate);

/**
 * Get current step for visualization (0-15).
 */
uint8_t ChopEngine_getCurrentStep(void);

/**
 * Get pattern for visualization.
 */
const bool* ChopEngine_getPattern(void);

/**
 * Get current density.
 */
float ChopEngine_getDensity(void);

#endif // CHOP_ENGINE_H