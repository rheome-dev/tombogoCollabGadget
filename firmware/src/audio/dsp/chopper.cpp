/**
 * Chopper DSP Module - Stub Implementation
 */

#include "chopper.h"
#include <Arduino.h>
#include <string.h>

void Chopper_init(void) {
    Serial.println("Chopper: initialized (stub)");
}

void Chopper_process(const int16_t* input, int16_t* output, uint32_t samples) {
    // Stub: pass-through
    if (input && output) {
        memcpy(output, input, samples * sizeof(int16_t));
    }
}

void Chopper_setTempo(uint16_t bpm) {
    (void)bpm;
}

void Chopper_setDensity(float density) {
    (void)density;
}

void Chopper_setRandomness(float random) {
    (void)random;
}

void Chopper_setPattern(ChopPattern pattern) {
    (void)pattern;
}

void Chopper_trigger(void) {
}
