/**
 * Core Audio Engine Implementation
 *
 * Stub implementation - DSP engines to be added
 */

#include "audio_engine.h"
#include "../config.h"
#include "../../hal/audio_hal.h"
#include "retroactive_buffer.h"
#include "../audio/dsp/resonator.h"
#include "../audio/dsp/chopper.h"
#include "../audio/dsp/synth.h"
#include "../audio/dsp/drums.h"
#include "../audio/dsp/effects_chain.h"
#include <Arduino.h>
#include <math.h>

// Audio processing buffers
static int16_t micBuffer[AUDIO_BUFFER_SIZE * 2];
static int16_t outputBuffer[AUDIO_BUFFER_SIZE * 2];
static int16_t loopBuffer[AUDIO_BUFFER_SIZE * 2];

// Current IMU data
static IMUData currentIMU;

// Engine states
static bool isRecording = false;
static bool isPlaying = false;
static uint8_t currentScaleType = SCALE_PENTATONIC_MAJOR;
static uint8_t currentRootNote = 0;  // C
static uint16_t currentTempo = 120;

void AudioEngine_init() {
    Serial.println("Initializing Audio Engine...");

    // Initialize DSP engines
    Resonator_init();
    Chopper_init();
    Synth_init();
    Drums_init();
    EffectsChain_init();

    // Initialize retroactive buffer
    RetroactiveBuffer_init();

    Serial.println("Audio Engine initialized");
}

void AudioEngine_process() {
    // Read microphone input
    uint32_t samplesRead = AudioHAL_readMic(micBuffer, AUDIO_BUFFER_SIZE);

    if (samplesRead > 0) {
        // Feed into retroactive buffer
        RetroactiveBuffer_write(micBuffer, samplesRead);

        // Process through DSP chain
        AudioEngine_processBuffer(micBuffer, outputBuffer, samplesRead);

        // Output to speaker
        AudioHAL_writeSpeaker(outputBuffer, samplesRead);
    }
}

void AudioEngine_processBuffer(const int16_t* input, int16_t* output, uint32_t samples) {
    // Start with mic pass-through
    for (uint32_t i = 0; i < samples * 2; i++) {
        output[i] = input[i];
    }

    // Mix in loop playback if active
    if (RetroactiveBuffer_isPlaying()) {
        uint32_t loopSamples = RetroactiveBuffer_read(loopBuffer, samples);
        if (loopSamples > 0) {
            // Pass loop through chopper DSP (placeholder until Faust is wired)
            Chopper_process(loopBuffer, loopBuffer, loopSamples);

            // Additive mix: scale both to avoid clipping
            // Mic at 65%, loop at 100% — adjust LOOP_MIX_GAIN to taste
            for (uint32_t i = 0; i < samples * 2; i++) {
                int32_t mixed = (int32_t)((int32_t)output[i] * 65 / 100)
                              + (int32_t)loopBuffer[i];
                // Soft clip
                if (mixed > 16383)  mixed = 16383;
                if (mixed < -16384) mixed = -16384;
                output[i] = (int16_t)mixed;
            }
        }
    }
}

void AudioEngine_setIMU(IMUData imuData) {
    currentIMU = imuData;

    // Map IMU to synthesis parameters
    // X-axis: filter cutoff
    // Y-axis: pitch bend
    Resonator_setFilterCutoff(80.0f + powf((imuData.ax + 1.0f) / 2.0f, 2.0f) * 8000.0f);
    Synth_setPitchBend((imuData.ay) * 0.5f);  // +/- half semitone
}

void AudioEngine_setScale(uint8_t scaleType, uint8_t rootNote) {
    currentScaleType = scaleType;
    currentRootNote = rootNote;
    Resonator_setScale(scaleType, rootNote);
    Synth_setScale(scaleType, rootNote);
}

void AudioEngine_setTempo(uint16_t bpm) {
    currentTempo = bpm;
    Drums_setTempo(bpm);
    Chopper_setTempo(bpm);
}

void AudioEngine_triggerCapture() {
    RetroactiveBuffer_capture();
}

void AudioEngine_setRecording(bool recording) {
    isRecording = recording;
}

void AudioEngine_setPlaying(bool playing) {
    isPlaying = playing;
}
