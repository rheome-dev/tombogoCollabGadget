/**
 * Retroactive Record Buffer
 *
 * Always-running circular buffer that captures audio for retroactive recording.
 * User can "go back in time" to capture the last N seconds.
 */

#ifndef RETROACTIVE_BUFFER_H
#define RETROACTIVE_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize retroactive buffer
 */
void RetroactiveBuffer_init(void);

/**
 * Write samples to buffer (called from audio ISR/task)
 */
void RetroactiveBuffer_write(const int16_t* samples, uint32_t count);

/**
 * Capture current buffer content to loop memory
 * Call this when user wants to save "the last few seconds"
 */
void RetroactiveBuffer_capture(void);

/**
 * Get captured loop data
 */
const int16_t* RetroactiveBuffer_getCaptured(void);

/**
 * Get captured loop length in samples
 */
uint32_t RetroactiveBuffer_getCapturedLength(void);

/**
 * Get current buffer fill level (0.0 to 1.0)
 */
float RetroactiveBuffer_getFillLevel(void);

/**
 * Clear captured loop
 */
void RetroactiveBuffer_clear(void);

/**
 * Set buffer length in seconds
 */
void RetroactiveBuffer_setLength(uint8_t seconds);

// ─── Playback ────────────────────────────────────────────────────────────────

/**
 * Start playing the captured loop from the beginning
 */
void RetroactiveBuffer_startPlayback(void);

/**
 * Stop loop playback
 */
void RetroactiveBuffer_stopPlayback(void);

/**
 * @return true if loop playback is active
 */
bool RetroactiveBuffer_isPlaying(void);

/**
 * Read the next chunk of loop playback samples.
 * Wraps around to the beginning when the loop end is reached.
 *
 * @param output Output buffer (stereo interleaved, samples*2 entries)
 * @param samples Number of mono samples to read
 * @return Number of samples actually read (may be less at loop boundary)
 */
uint32_t RetroactiveBuffer_read(int16_t* output, uint32_t samples);

#endif // RETROACTIVE_BUFFER_H
