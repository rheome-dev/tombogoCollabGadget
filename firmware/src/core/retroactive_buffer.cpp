/**
 * Retroactive Record Buffer Implementation
 *
 * Mono circular buffer for always-on recording.
 * All internal storage is mono int16_t.
 */

#include "retroactive_buffer.h"
#include "../config.h"
#include <Arduino.h>
#include <string.h>
#include <esp_heap_caps.h>

// Circular buffer for continuous recording (MONO)
static int16_t* circularBuffer = NULL;
static uint32_t bufferSize = 0;       // in mono samples
static uint32_t writeIndex = 0;

// Captured loop (what user saved) — MONO
static int16_t* capturedLoop = NULL;
static uint32_t capturedLength = 0;   // in mono samples
static bool hasCapture = false;

// Playback state
static uint32_t playIndex = 0;
static bool isPlaying = false;

static uint8_t bufferLengthSeconds = RETROACTIVE_BUFFER_SEC;

void RetroactiveBuffer_init() {
    bufferSize = SAMPLE_RATE * bufferLengthSeconds;  // mono samples

    Serial.printf("Retroactive buffer: Allocating %d seconds (%u mono samples, %u bytes)...\n",
                  bufferLengthSeconds, (unsigned int)bufferSize,
                  (unsigned int)(bufferSize * sizeof(int16_t)));

    // PSRAM-only allocation - fail if not available
    if (psramFound()) {
        circularBuffer = (int16_t*)heap_caps_malloc(bufferSize * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (circularBuffer) {
            Serial.printf("  Allocated in PSRAM (%u bytes)\n", bufferSize * sizeof(int16_t));
        }
    }

    if (!circularBuffer) {
        Serial.println("ERROR: Failed to allocate retroactive buffer in PSRAM!");
        Serial.printf("  PSRAM available: %u bytes, needed: %u\n",
                      ESP.getFreePsram(), (unsigned int)(bufferSize * sizeof(int16_t)));
        return;  // No fallback - fail hard
    }

    memset(circularBuffer, 0, bufferSize * sizeof(int16_t));
    Serial.printf("  Buffer ready: %u mono samples\n", (unsigned int)bufferSize);

    // Captured loop buffer (same max size)
    if (psramFound()) {
        capturedLoop = (int16_t*)heap_caps_malloc(bufferSize * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (capturedLoop) {
            Serial.printf("  Captured loop buffer: %u bytes in PSRAM\n", bufferSize * sizeof(int16_t));
        }
    }

    if (!capturedLoop) {
        Serial.println("ERROR: Failed to allocate captured loop buffer in PSRAM!");
        return;
    }

    memset(capturedLoop, 0, bufferSize * sizeof(int16_t));
    Serial.printf("  Retroactive buffer init complete\n");
}

void RetroactiveBuffer_write(const int16_t* samples, uint32_t count) {
    if (!circularBuffer) return;

    for (uint32_t i = 0; i < count; i++) {
        circularBuffer[writeIndex] = samples[i];
        writeIndex = (writeIndex + 1) % bufferSize;
    }
}

void RetroactiveBuffer_capture() {
    if (!circularBuffer || !capturedLoop) return;

    // Copy entire circular buffer content in chronological order
    // writeIndex points to the oldest sample
    uint32_t readIdx = writeIndex;
    for (uint32_t i = 0; i < bufferSize; i++) {
        capturedLoop[i] = circularBuffer[readIdx];
        readIdx = (readIdx + 1) % bufferSize;
    }

    capturedLength = bufferSize;
    hasCapture = true;

    Serial.printf("Captured loop: %u mono samples (%.1f sec)\n",
                  (unsigned int)capturedLength,
                  (float)capturedLength / SAMPLE_RATE);
}

const int16_t* RetroactiveBuffer_getCaptured() {
    return capturedLoop;
}

uint32_t RetroactiveBuffer_getCapturedLength() {
    return capturedLength;
}

float RetroactiveBuffer_getFillLevel() {
    return hasCapture ? 1.0f : 0.5f;
}

void RetroactiveBuffer_clear() {
    if (capturedLoop) {
        memset(capturedLoop, 0, bufferSize * sizeof(int16_t));
    }
    capturedLength = 0;
    hasCapture = false;
    isPlaying = false;
    playIndex = 0;
}

void RetroactiveBuffer_setLength(uint8_t seconds) {
    bufferLengthSeconds = seconds;
}

// ─── Playback (MONO) ────────────────────────────────────────────────────────

void RetroactiveBuffer_startPlayback() {
    if (!hasCapture) return;
    playIndex = 0;
    isPlaying = true;
    Serial.println("Loop playback started");
}

void RetroactiveBuffer_stopPlayback() {
    isPlaying = false;
    Serial.println("Loop playback stopped");
}

bool RetroactiveBuffer_isPlaying() {
    return isPlaying && hasCapture;
}

uint32_t RetroactiveBuffer_read(int16_t* output, uint32_t samples) {
    if (!isPlaying || !hasCapture || !capturedLoop) return 0;

    for (uint32_t i = 0; i < samples; i++) {
        output[i] = capturedLoop[playIndex];
        playIndex++;
        if (playIndex >= capturedLength) {
            playIndex = 0;
        }
    }

    return samples;
}
