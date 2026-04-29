/**
 * Retroactive Record Buffer
 *
 * Always-running mono circular buffer for retroactive recording.
 * All storage and I/O is mono int16_t.
 */

#ifndef RETROACTIVE_BUFFER_H
#define RETROACTIVE_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

void RetroactiveBuffer_init(void);

/**
 * Write mono samples to the circular buffer.
 */
void RetroactiveBuffer_write(const int16_t* samples, uint32_t count);

void RetroactiveBuffer_capture(void);

const int16_t* RetroactiveBuffer_getCaptured(void);
uint32_t RetroactiveBuffer_getCapturedLength(void);

float RetroactiveBuffer_getFillLevel(void);
void RetroactiveBuffer_clear(void);
void RetroactiveBuffer_setLength(uint8_t seconds);

void RetroactiveBuffer_startPlayback(void);
void RetroactiveBuffer_stopPlayback(void);
bool RetroactiveBuffer_isPlaying(void);

/**
 * Read the next chunk of mono loop playback samples.
 * Wraps around at loop end.
 *
 * @param output  Mono output buffer
 * @param samples Number of mono samples to read
 * @return Number of samples read
 */
uint32_t RetroactiveBuffer_read(int16_t* output, uint32_t samples);

#endif // RETROACTIVE_BUFFER_H
