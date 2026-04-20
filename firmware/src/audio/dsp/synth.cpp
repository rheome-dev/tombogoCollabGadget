/**
 * Synth DSP Module - Stub Implementation
 */

#include "synth.h"
#include <Arduino.h>
#include <string.h>

void Synth_init(void) {
    Serial.println("Synth: initialized (stub)");
}

void Synth_process(int16_t* output, uint32_t samples) {
    // Stub: silence
    if (output) {
        memset(output, 0, samples * sizeof(int16_t));
    }
}

void Synth_noteOn(uint8_t note, uint8_t velocity) {
    (void)note;
    (void)velocity;
}

void Synth_noteOff(void) {
}

void Synth_setWaveform(Waveform wave) {
    (void)wave;
}

void Synth_setEnvelope(float attack, float decay, float sustain, float release) {
    (void)attack;
    (void)decay;
    (void)sustain;
    (void)release;
}

void Synth_setPitchBend(float semitones) {
    (void)semitones;
}

void Synth_setScale(uint8_t scaleType, uint8_t rootNote) {
    (void)scaleType;
    (void)rootNote;
}

float Synth_getFrequency(void) {
    return 440.0f;
}
