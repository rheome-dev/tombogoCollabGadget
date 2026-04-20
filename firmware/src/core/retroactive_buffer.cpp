/**
 * Retroactive Record Buffer Implementation
 */

#include "retroactive_buffer.h"
#include "../config.h"
#include <Arduino.h>
#include <string.h>
#include <esp_heap_caps.h>

// Circular buffer for continuous recording
static int16_t* circularBuffer = NULL;
static uint32_t bufferSize = 0;
static uint32_t writeIndex = 0;

// Captured loop (what user saved)
static int16_t* capturedLoop = NULL;
static uint32_t capturedLength = 0;
static bool hasCapture = false;

// Playback state
static uint32_t playIndex = 0;
static bool isPlaying = false;

// Current settings
static uint8_t bufferLengthSeconds = RETROACTIVE_BUFFER_SEC;

void RetroactiveBuffer_init() {
    bufferSize = SAMPLE_RATE * bufferLengthSeconds * 2;  // *2 for stereo

    Serial.printf("Retroactive buffer: Allocating %d seconds (%u bytes)...\n",
                  bufferLengthSeconds, (unsigned int)(bufferSize * sizeof(int16_t)));

    // Allocate from PSRAM if available, else DRAM
#if PSRAM_AVAILABLE
    if (psramFound()) {
        circularBuffer = (int16_t*)heap_caps_malloc(bufferSize * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (circularBuffer) {
            Serial.println("  Allocated in PSRAM");
        }
    }
#endif

    // Fallback to regular DRAM
    if (!circularBuffer) {
        circularBuffer = (int16_t*)malloc(bufferSize * sizeof(int16_t));
        if (circularBuffer) {
            Serial.println("  Allocated in DRAM");
        }
    }

    if (circularBuffer) {
        memset(circularBuffer, 0, bufferSize * sizeof(int16_t));
        Serial.printf("  Buffer ready: %u samples\n", (unsigned int)bufferSize);
    } else {
        Serial.println("ERROR: Failed to allocate retroactive buffer!");
    }

    // Allocate captured loop buffer (max size) - also try PSRAM first
#if PSRAM_AVAILABLE
    if (psramFound()) {
        capturedLoop = (int16_t*)heap_caps_malloc(bufferSize * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    }
#endif
    if (!capturedLoop) {
        capturedLoop = (int16_t*)malloc(bufferSize * sizeof(int16_t));
    }

    if (capturedLoop) {
        memset(capturedLoop, 0, bufferSize * sizeof(int16_t));
    }
}

void RetroactiveBuffer_write(const int16_t* samples, uint32_t count) {
    if (!circularBuffer) return;

    for (uint32_t i = 0; i < count * 2; i++) {  // *2 for stereo
        circularBuffer[writeIndex] = samples[i];
        writeIndex = (writeIndex + 1) % bufferSize;
    }
}

void RetroactiveBuffer_capture() {
    if (!circularBuffer || !capturedLoop) return;

    // Copy from circular buffer to captured loop
    // We capture from current write position backwards
    uint32_t captureLen = bufferSize;  // Full buffer

    // Read backwards from writeIndex
    uint32_t readIdx = (writeIndex + bufferSize - 1) % bufferSize;

    for (uint32_t i = 0; i < captureLen; i++) {
        capturedLoop[i] = circularBuffer[readIdx];
        readIdx = (readIdx + bufferSize - 1) % bufferSize;
    }

    capturedLength = captureLen;
    hasCapture = true;

    Serial.printf("Captured loop: %u samples\n", (unsigned int)capturedLength);
}

const int16_t* RetroactiveBuffer_getCaptured() {
    return capturedLoop;
}

uint32_t RetroactiveBuffer_getCapturedLength() {
    return capturedLength;
}

float RetroactiveBuffer_getFillLevel() {
    // For circular buffer, we always have data
    // This could track how full the "meaningful content" is
    return hasCapture ? 1.0f : 0.5f;
}

void RetroactiveBuffer_clear() {
    if (capturedLoop) {
        memset(capturedLoop, 0, bufferSize * sizeof(int16_t));
    }
    capturedLength = 0;
    hasCapture = false;
}

void RetroactiveBuffer_setLength(uint8_t seconds) {
    bufferLengthSeconds = seconds;
    // Would need to reallocate - for now just update config
    // TODO: Implement dynamic reallocation
}

// ─── Playback ────────────────────────────────────────────────────────────────

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

    // samples is mono count; buffer is stereo interleaved (L,R per sample pair)
    uint32_t monoRead = 0;

    for (uint32_t i = 0; i < samples; i++) {
        uint32_t idx = playIndex + i;
        if (idx >= capturedLength) {
            // Wrap around
            playIndex = 0;
            idx = i;
            // Chopper_trigger() would go here when Faust chop is wired
        }
        // Output mono sample to both channels (mono loop → stereo out)
        output[i * 2]     = capturedLoop[idx * 2];     // left
        output[i * 2 + 1] = capturedLoop[idx * 2 + 1]; // right
        monoRead++;
    }

    playIndex += samples;
    if (playIndex >= capturedLength) {
        playIndex = 0;  // Wrap
    }

    return monoRead;
}
