/**
 * Drums DSP Module - Stub Implementation
 */

#include "drums.h"
#include <Arduino.h>
#include <string.h>

void Drums_init(void) {
    Serial.println("Drums: initialized (stub)");
}

void Drums_process(int16_t* output, uint32_t samples) {
    // Stub: silence
    if (output) {
        memset(output, 0, samples * sizeof(int16_t));
    }
}

void Drums_setTempo(uint16_t bpm) {
    (void)bpm;
}

void Drums_setPattern(DrumSound drum, uint16_t pattern) {
    (void)drum;
    (void)pattern;
}

void Drums_setVolume(DrumSound drum, float volume) {
    (void)drum;
    (void)volume;
}

void Drums_start(void) {
}

void Drums_stop(void) {
}

void Drums_step(void) {
}
