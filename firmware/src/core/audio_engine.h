/**
 * Core Audio Engine
 *
 * Central hub for all audio processing: mixing, routing, and effects
 */

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <stdint.h>
#include "hal/imu_hal.h"

// Forward declarations
struct IMUData;

/**
 * Audio Engine initialization
 */
void AudioEngine_init(void);

/**
 * Main audio processing callback
 * Called at sample rate from I2S DMA
 */
void AudioEngine_process(void);

/**
 * Process a buffer of audio
 * @param input Input samples from microphone
 * @param output Output samples to speaker
 * @param samples Number of samples
 */
void AudioEngine_processBuffer(const int16_t* input, int16_t* output, uint32_t samples);

/**
 * Set IMU data for motion control
 */
void AudioEngine_setIMU(IMUData imuData);

/**
 * Set current scale
 */
void AudioEngine_setScale(uint8_t scaleType, uint8_t rootNote);

/**
 * Set tempo
 */
void AudioEngine_setTempo(uint16_t bpm);

/**
 * Trigger retroactive capture
 */
void AudioEngine_triggerCapture(void);

/**
 * Engine control
 */
void AudioEngine_setRecording(bool recording);
void AudioEngine_setPlaying(bool playing);

#endif // AUDIO_ENGINE_H
