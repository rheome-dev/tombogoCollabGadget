/**
 * Core Audio Engine Implementation
 *
 * Full mono signal chain:
 *   Mic → RetroactiveBuffer → [Loop Playback] → [Chop] → [Resonator] → Volume → Stereo → I2S
 *
 * Runs as a FreeRTOS task pinned to Core 1.
 */

#include "audio_engine.h"
#include "../config.h"
#include "../../hal/audio_hal.h"
#include "retroactive_buffer.h"
#include "bpm_clock.h"
#include "../audio/chop_engine.h"
#include "../audio/transient_detector.h"
#include "../audio/dsp/resonator.h"
#include "../audio/dsp/chopper.h"
#include "../audio/dsp/synth.h"
#include "../audio/dsp/drums.h"
#include "../audio/dsp/effects_chain.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <string.h>

// I2S buffers (stereo interleaved for hardware)
static int16_t i2sReadBuf[AUDIO_BUFFER_SIZE * 2];
static int16_t i2sWriteBuf[AUDIO_BUFFER_SIZE * 2];

// Mono working buffers
static int16_t monoMic[AUDIO_BUFFER_SIZE];
static int16_t monoLoop[AUDIO_BUFFER_SIZE];
static int16_t monoChop[AUDIO_BUFFER_SIZE];
static int16_t monoOut[AUDIO_BUFFER_SIZE];

static IMUData currentIMU;
static TaskHandle_t audioTaskHandle = NULL;
static uint32_t loopPlayCount = 0;     // tracks playback iterations
static bool wasPlaying = false;
static uint32_t micDiagCount = 0;
static int16_t micDiagMin = 0;
static int16_t micDiagMax = 0;

// Engine state
static bool chopEnabled = false;
static bool resonatorEnabled = false;
static float wetDryMix = 0.0f;
static uint8_t masterVolume = 80;  // 0-100
static int8_t pitchSemitones = 0;
static float pitchReadIncrement = 1.0f;
static float pitchReadPos = 0.0f;

// Startup test tone state
static bool testTonePlaying = false;
static uint32_t testToneSamplesLeft = 0;
static float testTonePhase = 0.0f;
static const float TEST_TONE_FREQ = 440.0f;
static const uint32_t TEST_TONE_DURATION_MS = 1500;

void AudioEngine_init() {
    Serial.println("Initializing Audio Engine...");

    Resonator_init();
    Chopper_init();
    Synth_init();
    Drums_init();
    EffectsChain_init();
    ChopEngine_init();

    RetroactiveBuffer_init();

    Serial.println("Audio Engine initialized");
}

static void audioTaskFunction(void* param) {
    (void)param;
    Serial.println("AudioTask: Started on Core 1");

    while (true) {
        AudioEngine_process();
        vTaskDelay(pdMS_TO_TICKS(AUDIO_TASK_DELAY_MS));
    }
}

void AudioEngine_startTask(void) {
    xTaskCreatePinnedToCore(
        audioTaskFunction,
        "AudioTask",
        AUDIO_TASK_STACK_SIZE,
        nullptr,
        AUDIO_TASK_PRIORITY,
        &audioTaskHandle,
        1  // Core 1
    );
    Serial.println("AudioTask: Created on Core 1");
}

void AudioEngine_playTestTone(void) {
    testTonePlaying = true;
    testToneSamplesLeft = (uint32_t)SAMPLE_RATE * TEST_TONE_DURATION_MS / 1000;
    testTonePhase = 0.0f;
    Serial.printf("AudioEngine: Playing %dHz test tone for %dms\n",
                  (int)TEST_TONE_FREQ, TEST_TONE_DURATION_MS);
}

void AudioEngine_process() {
    // If playing test tone, bypass normal signal chain
    if (testTonePlaying) {
        uint32_t count = AUDIO_BUFFER_SIZE;
        if (count > testToneSamplesLeft) count = testToneSamplesLeft;

        float phaseInc = 2.0f * 3.14159265f * TEST_TONE_FREQ / (float)SAMPLE_RATE;
        for (uint32_t i = 0; i < count; i++) {
            float s = sinf(testTonePhase) * 8000.0f;
            int16_t sample = (int16_t)s;
            i2sWriteBuf[i * 2]     = sample;
            i2sWriteBuf[i * 2 + 1] = sample;
            testTonePhase += phaseInc;
        }
        if (testTonePhase > 2.0f * 3.14159265f) {
            testTonePhase -= 2.0f * 3.14159265f;
        }

        AudioHAL_writeSpeaker(i2sWriteBuf, count);
        // Also drain mic input to keep I2S flowing
        AudioHAL_readMic(i2sReadBuf, AUDIO_BUFFER_SIZE);

        testToneSamplesLeft -= count;
        if (testToneSamplesLeft == 0) {
            testTonePlaying = false;
            Serial.println("AudioEngine: Test tone complete");
        }
        return;
    }

    // Read stereo I2S input
    uint32_t stereoSamples = AudioHAL_readMic(i2sReadBuf, AUDIO_BUFFER_SIZE);
    if (stereoSamples == 0) return;

    uint32_t monoCount = stereoSamples;

    // Extract mono from stereo (take left channel)
    for (uint32_t i = 0; i < monoCount; i++) {
        monoMic[i] = i2sReadBuf[i * 2];
    }

    // Tick BPM clock once per sample
    for (uint32_t i = 0; i < monoCount; i++) {
        BPMClock_tick();
    }

    // Mic level diagnostics (log every 2 seconds)
    for (uint32_t i = 0; i < monoCount; i++) {
        if (monoMic[i] < micDiagMin) micDiagMin = monoMic[i];
        if (monoMic[i] > micDiagMax) micDiagMax = monoMic[i];
    }
    micDiagCount += monoCount;
    if (micDiagCount >= SAMPLE_RATE * 2) {
        Serial.printf("[MIC] min=%d max=%d range=%d (silence < 100, noise > 1000)\n",
                      micDiagMin, micDiagMax, micDiagMax - micDiagMin);
        micDiagMin = 0;
        micDiagMax = 0;
        micDiagCount = 0;
    }

    // Feed mono mic into retroactive buffer
    RetroactiveBuffer_write(monoMic, monoCount);

    // Start with mic input as output
    memcpy(monoOut, monoMic, monoCount * sizeof(int16_t));

    // Mix in loop playback if active (with pitch shift)
    bool isPlaying = RetroactiveBuffer_isPlaying();
    if (isPlaying && !wasPlaying) {
        Serial.printf("[LOOP] Playback started - capturing %u samples\n",
                      (unsigned int)RetroactiveBuffer_getCapturedLength());
    }
    wasPlaying = isPlaying;

    if (isPlaying) {
        loopPlayCount += monoCount;
        if (loopPlayCount >= SAMPLE_RATE) {
            Serial.printf("[LOOP] Playing - %u samples/sec\n", (unsigned int)loopPlayCount);
            loopPlayCount = 0;
        }
        uint32_t loopSamples;
        if (pitchSemitones != 0) {
            // Pitch shift via playback rate change
            // Read more/fewer samples and resample
            const int16_t* captured = RetroactiveBuffer_getCaptured();
            uint32_t capturedLen = RetroactiveBuffer_getCapturedLength();
            if (captured && capturedLen > 0) {
                for (uint32_t i = 0; i < monoCount; i++) {
                    uint32_t idx0 = (uint32_t)pitchReadPos % capturedLen;
                    monoLoop[i] = captured[idx0];
                    pitchReadPos += pitchReadIncrement;
                    if (pitchReadPos >= (float)capturedLen) {
                        pitchReadPos -= (float)capturedLen;
                    }
                }
                loopSamples = monoCount;
            } else {
                loopSamples = 0;
            }
        } else {
            loopSamples = RetroactiveBuffer_read(monoLoop, monoCount);
        }

        if (loopSamples > 0) {
            if (chopEnabled) {
                uint32_t sliceLen = (uint32_t)SAMPLE_RATE * 60U / (BPMClock_getBPM() * 4U);
                ChopEngine_process(monoLoop, monoChop, monoCount, sliceLen);

                // Use chop output as the loop signal
                memcpy(monoLoop, monoChop, monoCount * sizeof(int16_t));
            }

            // Replace output with loop (mic muted during playback)
            for (uint32_t i = 0; i < monoCount; i++) {
                monoOut[i] = monoLoop[i];
            }
        }
    }

    // Apply master volume (0-100 → 0-256 gain)
    uint16_t volGain = (uint16_t)masterVolume * 256 / 100;
    for (uint32_t i = 0; i < monoCount; i++) {
        int32_t s = ((int32_t)monoOut[i] * volGain) >> 8;
        if (s > 32767)  s = 32767;
        if (s < -32768) s = -32768;
        monoOut[i] = (int16_t)s;
    }

    // Duplicate mono to stereo for I2S output
    for (uint32_t i = 0; i < monoCount; i++) {
        i2sWriteBuf[i * 2]     = monoOut[i];
        i2sWriteBuf[i * 2 + 1] = monoOut[i];
    }

    AudioHAL_writeSpeaker(i2sWriteBuf, monoCount);
}

void AudioEngine_processBuffer(const int16_t* input, int16_t* output, uint32_t samples) {
    for (uint32_t i = 0; i < samples; i++) {
        output[i] = input[i];
    }
}

void AudioEngine_setIMU(IMUData imuData) {
    currentIMU = imuData;
    Resonator_setFilterCutoff(80.0f + powf((imuData.ax + 1.0f) / 2.0f, 2.0f) * 8000.0f);
    Synth_setPitchBend((imuData.ay) * 0.5f);
}

void AudioEngine_setScale(uint8_t scaleType, uint8_t rootNote) {
    Resonator_setScale(scaleType, rootNote);
    Synth_setScale(scaleType, rootNote);
}

void AudioEngine_setTempo(uint16_t bpm) {
    Drums_setTempo(bpm);
    Chopper_setTempo(bpm);
    BPMClock_setBPM(bpm);
}

void AudioEngine_triggerCapture() {
    RetroactiveBuffer_capture();
}

void AudioEngine_setRecording(bool recording) {
    (void)recording;
}

void AudioEngine_setPlaying(bool playing) {
    if (playing) {
        RetroactiveBuffer_startPlayback();
    } else {
        RetroactiveBuffer_stopPlayback();
    }
}

void AudioEngine_setChopEnabled(bool enabled) {
    chopEnabled = enabled;
    ChopEngine_setActive(enabled);
    Serial.printf("AudioEngine: Chop %s\n", enabled ? "ON" : "OFF");
}

void AudioEngine_setResonatorEnabled(bool enabled) {
    resonatorEnabled = enabled;
    Serial.printf("AudioEngine: Resonator %s\n", enabled ? "ON" : "OFF");
}

void AudioEngine_setDensity(float density) {
    ChopEngine_setDensity(density);
}

void AudioEngine_setWetDry(float mix) {
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    wetDryMix = mix;
}

void AudioEngine_setVolume(uint8_t vol) {
    if (vol > 100) vol = 100;
    masterVolume = vol;
}

void AudioEngine_setPitch(int8_t semitones) {
    if (semitones < -12) semitones = -12;
    if (semitones > 12) semitones = 12;
    pitchSemitones = semitones;
    pitchReadIncrement = powf(2.0f, (float)semitones / 12.0f);
    Serial.printf("AudioEngine: Pitch %+d semitones (rate %.3f)\n", semitones, pitchReadIncrement);
}
