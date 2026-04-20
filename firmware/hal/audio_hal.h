/**
 * Hardware Abstraction Layer - Audio Interface
 *
 * Defines the contract for audio input/output hardware.
 */

#ifndef AUDIO_HAL_H
#define AUDIO_HAL_H

#include <stdint.h>

/**
 * Audio HAL initialization
 */
void AudioHAL_init(void);

/**
 * Start audio playback/capture
 */
void AudioHAL_start(void);

/**
 * Stop audio playback/capture
 */
void AudioHAL_stop(void);

/**
 * Set volume (0-100)
 */
void AudioHAL_setVolume(uint8_t volume);

/**
 * Read audio samples from microphone
 * @param buffer Buffer to fill
 * @param samples Number of samples to read
 * @return Number of samples actually read
 */
uint32_t AudioHAL_readMic(int16_t* buffer, uint32_t samples);

/**
 * Write audio samples to speaker/headphones
 * @param buffer Buffer containing samples
 * @param samples Number of samples to write
 */
void AudioHAL_writeSpeaker(const int16_t* buffer, uint32_t samples);

/**
 * Check if audio input is available
 */
bool AudioHAL_micAvailable(void);

/**
 * Check if audio output is ready
 */
bool AudioHAL_speakerReady(void);

#endif // AUDIO_HAL_H
