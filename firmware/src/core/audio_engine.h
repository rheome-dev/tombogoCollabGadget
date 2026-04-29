/**
 * Core Audio Engine
 *
 * Central hub for all audio processing: mixing, routing, and effects.
 * Runs on Core 1 as a high-priority FreeRTOS task.
 */

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <stdint.h>
#include "hal/imu_hal.h"

struct IMUData;

void AudioEngine_init(void);

/**
 * Start the Core 1 audio task.
 * Must be called after AudioEngine_init() and all DSP engines are ready.
 */
void AudioEngine_startTask(void);

void AudioEngine_process(void);
void AudioEngine_processBuffer(const int16_t* input, int16_t* output, uint32_t samples);

void AudioEngine_setIMU(IMUData imuData);
void AudioEngine_setScale(uint8_t scaleType, uint8_t rootNote);
void AudioEngine_setTempo(uint16_t bpm);
void AudioEngine_triggerCapture(void);
void AudioEngine_setRecording(bool recording);
void AudioEngine_setPlaying(bool playing);

void AudioEngine_setChopEnabled(bool enabled);
void AudioEngine_setResonatorEnabled(bool enabled);
void AudioEngine_setDensity(float density);
void AudioEngine_setWetDry(float mix);
void AudioEngine_setVolume(uint8_t vol);
void AudioEngine_setPitch(int8_t semitones);
void AudioEngine_playTestTone(void);

#endif // AUDIO_ENGINE_H
