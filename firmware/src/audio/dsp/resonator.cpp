/**
 * Resonator DSP Module - Stub Implementation
 */

#include "resonator.h"
#include <Arduino.h>

void Resonator_init(void) {
    Serial.println("Resonator: initialized (stub)");
}

void Resonator_process(const int16_t* input, int16_t* output, uint32_t samples) {
    // Stub: pass-through
    if (input && output) {
        memcpy(output, input, samples * sizeof(int16_t));
    }
}

void Resonator_setFilterCutoff(float cutoffHz) {
    (void)cutoffHz;
}

void Resonator_setResonance(float q) {
    (void)q;
}

void Resonator_setDecay(float seconds) {
    (void)seconds;
}

void Resonator_setScale(uint8_t scaleType, uint8_t rootNote) {
    (void)scaleType;
    (void)rootNote;
}

void Resonator_setMode(ResonatorMode mode) {
    (void)mode;
}
