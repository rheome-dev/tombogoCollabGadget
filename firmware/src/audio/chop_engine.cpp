/**
 * Chop Engine Implementation
 *
 * Bjorklund Euclidean pattern with real slice playback:
 * - Normal: forward with 2ms attack ramp
 * - Reverse: backward playback
 * - Ratchet: 2-3 rapid repetitions at 32nd-note spacing
 */

#include "chop_engine.h"
#include "../config.h"
#include "../core/retroactive_buffer.h"
#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// 2ms attack ramp at 16kHz = 32 samples
#define ATTACK_RAMP_SAMPLES 32

static ChopState state;

// ─── Bjorklund Algorithm ─────────────────────────────────────────────────────

static void bjorklund(uint8_t pulses, uint8_t steps, bool* pattern) {
    if (steps == 0) return;
    memset(pattern, 0, steps);

    if (pulses == 0) return;
    if (pulses >= steps) {
        memset(pattern, 1, steps);
        return;
    }

    // Bresenham-style Euclidean distribution
    int16_t error = 0;
    for (uint8_t i = 0; i < steps; i++) {
        error += pulses;
        if (error >= steps) {
            error -= steps;
            pattern[i] = true;
        }
    }
}

static uint8_t randomPlaybackType(void) {
    uint8_t r = rand() % 100;
    if (r < 77) return PLAY_NORMAL;
    if (r < 92) return PLAY_REVERSE;
    return PLAY_RATCHET;
}

// Read a slice from the captured loop buffer, applying playback type
static void readSlice(int16_t* output, uint32_t outLen,
                      const int16_t* loopBuf, uint32_t loopLen,
                      uint32_t sliceStart, uint32_t sliceLen,
                      uint8_t playbackType) {
    if (!loopBuf || loopLen == 0 || sliceLen == 0) {
        memset(output, 0, outLen * sizeof(int16_t));
        return;
    }

    switch (playbackType) {
        case PLAY_NORMAL: {
            for (uint32_t i = 0; i < outLen; i++) {
                uint32_t idx = (sliceStart + (i % sliceLen)) % loopLen;
                int16_t sample = loopBuf[idx];

                // 2ms attack ramp
                if (i < ATTACK_RAMP_SAMPLES) {
                    sample = (int16_t)((int32_t)sample * i / ATTACK_RAMP_SAMPLES);
                }
                output[i] = sample;
            }
            break;
        }

        case PLAY_REVERSE: {
            for (uint32_t i = 0; i < outLen; i++) {
                uint32_t reversePos = sliceLen - 1 - (i % sliceLen);
                uint32_t idx = (sliceStart + reversePos) % loopLen;
                int16_t sample = loopBuf[idx];

                if (i < ATTACK_RAMP_SAMPLES) {
                    sample = (int16_t)((int32_t)sample * i / ATTACK_RAMP_SAMPLES);
                }
                output[i] = sample;
            }
            break;
        }

        case PLAY_RATCHET: {
            // 2-3 rapid repetitions within the step duration
            uint8_t reps = 2 + (rand() % 2);
            uint32_t repLen = outLen / reps;
            if (repLen == 0) repLen = 1;

            // Play a shorter portion of the slice for each rep
            uint32_t readLen = sliceLen < repLen ? sliceLen : repLen;

            for (uint32_t i = 0; i < outLen; i++) {
                uint32_t posInRep = i % repLen;
                if (posInRep < readLen) {
                    uint32_t idx = (sliceStart + posInRep) % loopLen;
                    int16_t sample = loopBuf[idx];

                    if (posInRep < ATTACK_RAMP_SAMPLES) {
                        sample = (int16_t)((int32_t)sample * posInRep / ATTACK_RAMP_SAMPLES);
                    }
                    output[i] = sample;
                } else {
                    output[i] = 0;
                }
            }
            break;
        }

        default:
            memset(output, 0, outLen * sizeof(int16_t));
            break;
    }
}

// ─── Public API ─────────────────────────────────────────────────────────────

void ChopEngine_init(void) {
    memset(&state, 0, sizeof(state));
    state.density = 0.5f;
    state.steps = 16;
    state.active = false;
    state.mute = true;
    state.fadeOut = false;
    state.fadeSamples = 0;
    state.currentStep = 0;
    state.stepSamples = 2000;
    state.sampleCounter = 0;

    // Compute the initial Euclidean pattern so the engine isn't silent the
    // first time it's enabled at the default density (otherwise setDensity()
    // early-returns when the requested value matches the init value).
    uint8_t pulses = 2 + (uint8_t)(powf(state.density, 0.7f) * 14.0f);
    if (pulses < 2) pulses = 2;
    if (pulses > 16) pulses = 16;
    bjorklund(pulses, state.steps, state.pattern);
    for (uint8_t i = 0; i < state.steps; i++) {
        state.playbackType[i] = state.pattern[i] ? randomPlaybackType() : (uint8_t)PLAY_NORMAL;
    }
}

void ChopEngine_setSlices(const SliceInfo* slices, uint8_t count) {
    if (!slices || count == 0) return;

    uint8_t n = count > MAX_SLICES ? MAX_SLICES : count;
    memcpy(state.slices, slices, n * sizeof(SliceInfo));
    state.numSlices = n;

    Serial.printf("ChopEngine: %u slices loaded\n", n);
}

void ChopEngine_setDensity(float density) {
    if (density < 0) density = 0;
    if (density > 1) density = 1;

    if (density == state.density) return;
    state.density = density;

    uint8_t pulses = 2 + (uint8_t)(powf(density, 0.7f) * 14.0f);
    if (pulses < 2) pulses = 2;
    if (pulses > 16) pulses = 16;

    bjorklund(pulses, state.steps, state.pattern);

    for (uint8_t i = 0; i < state.steps; i++) {
        if (state.pattern[i]) {
            state.playbackType[i] = randomPlaybackType();
        } else {
            state.playbackType[i] = PLAY_NORMAL;
        }
    }

    Serial.printf("ChopEngine: Density %.2f → %u pulses\n", density, pulses);
}

void ChopEngine_setActive(bool active) {
    if (active != state.active) {
        state.active = active;
        state.mute = !active;
        Serial.printf("ChopEngine: %s\n", active ? "active" : "inactive");
    }
}

void ChopEngine_process(const int16_t* input, int16_t* output,
                         uint32_t samples, uint32_t sliceLen) {
    if (!input || !output || samples == 0) return;

    state.stepSamples = sliceLen;

    if (!state.active || state.numSlices == 0) {
        if (!state.mute) {
            memcpy(output, input, samples * sizeof(int16_t));
        } else {
            memset(output, 0, samples * sizeof(int16_t));
        }
        return;
    }

    const int16_t* loopBuf = RetroactiveBuffer_getCaptured();
    uint32_t loopLen = RetroactiveBuffer_getCapturedLength();

    // Process sample-by-sample, advancing step on sixteenth-note boundaries
    for (uint32_t i = 0; i < samples; i++) {
        // Check if we crossed a step boundary
        if (state.sampleCounter >= state.stepSamples) {
            state.sampleCounter -= state.stepSamples;
            state.currentStep = (state.currentStep + 1) % state.steps;
        }

        bool play = state.pattern[state.currentStep];

        if (play && loopBuf && loopLen > 0) {
            uint8_t sliceIdx = state.currentStep % state.numSlices;
            uint32_t sliceStart = state.slices[sliceIdx].onsetSample;
            uint32_t sLen = state.slices[sliceIdx].sliceLength;
            if (sLen == 0) sLen = state.stepSamples;

            // Position within current step
            uint32_t posInStep = state.sampleCounter;
            uint32_t idx;
            uint8_t ptype = state.playbackType[state.currentStep];

            if (ptype == PLAY_REVERSE) {
                uint32_t reversePos = (sLen > 0) ? (sLen - 1 - (posInStep % sLen)) : 0;
                idx = (sliceStart + reversePos) % loopLen;
            } else {
                idx = (sliceStart + (posInStep % sLen)) % loopLen;
            }

            int16_t sample = loopBuf[idx];

            // 2ms attack ramp at step start
            if (posInStep < ATTACK_RAMP_SAMPLES) {
                sample = (int16_t)((int32_t)sample * posInStep / ATTACK_RAMP_SAMPLES);
            }

            // Ratchet: repeat at double speed (every half-step)
            if (ptype == PLAY_RATCHET) {
                uint32_t halfStep = state.stepSamples / 2;
                if (halfStep > 0) {
                    uint32_t ratchetPos = posInStep % halfStep;
                    idx = (sliceStart + (ratchetPos % sLen)) % loopLen;
                    sample = loopBuf[idx];
                    if (ratchetPos < ATTACK_RAMP_SAMPLES) {
                        sample = (int16_t)((int32_t)sample * ratchetPos / ATTACK_RAMP_SAMPLES);
                    }
                }
            }

            output[i] = sample;
        } else {
            output[i] = 0;
        }

        state.sampleCounter++;
    }
}

void ChopEngine_randomize(void) {
    for (uint8_t i = 0; i < state.steps; i++) {
        if (state.pattern[i]) {
            state.playbackType[i] = randomPlaybackType();
        }
    }
    Serial.println("ChopEngine: Pattern randomized");
}

uint8_t ChopEngine_getCurrentStep(void) {
    return state.currentStep;
}

const bool* ChopEngine_getPattern(void) {
    return state.pattern;
}

float ChopEngine_getDensity(void) {
    return state.density;
}
