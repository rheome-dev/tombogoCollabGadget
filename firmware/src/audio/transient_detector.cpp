/**
 * Transient Detector Implementation
 *
 * Amplitude-based onset detection using RMS envelope tracking.
 * No FFT - simple energy-based approach suitable for ESP32.
 */

#include "transient_detector.h"
#include "../config.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

// Frame size for RMS computation (256 samples = 16ms at 16kHz)
#define RMS_FRAME_SIZE      256
// Smoothing factor for envelope follower
#define ENVELOPE_ALPHA      0.95f
// Noise floor (RMS threshold)
#define NOISE_FLOOR         500  // int16_t RMS units
// Max RMS frames analyzed (~16s at 16kHz, 256-sample frames)
#define MAX_RMS_FRAMES      1024

// File-scope to keep this 4KB working buffer off the caller's task stack
// (InputTask only has 4KB total). Single-threaded use: detection runs only
// on STAGE_CHOP entry, serialized through the input handler.
static float rmsFrames[MAX_RMS_FRAMES];

static void computeRMSFrame(const int16_t* buffer, uint32_t length,
                             uint32_t frameIdx, float* rmsOut) {
    uint32_t start = frameIdx * RMS_FRAME_SIZE;
    if (start >= length) {
        *rmsOut = 0;
        return;
    }

    uint32_t count = RMS_FRAME_SIZE;
    if (start + count > length) {
        count = length - start;
    }

    int64_t sumSquares = 0;
    for (uint32_t i = 0; i < count; i++) {
        int32_t s = buffer[start + i];
        sumSquares += (int64_t)s * s;
    }

    float rms = sqrtf((float)sumSquares / count);
    *rmsOut = rms;
}

void TransientDetector_detect(const int16_t* buffer, uint32_t length,
                               TransientResult* result) {
    memset(result, 0, sizeof(TransientResult));

    if (!buffer || length < RMS_FRAME_SIZE) return;

    // Compute number of RMS frames
    uint32_t numFrames = length / RMS_FRAME_SIZE;

    if (numFrames > MAX_RMS_FRAMES) numFrames = MAX_RMS_FRAMES;

    for (uint32_t f = 0; f < numFrames; f++) {
        computeRMSFrame(buffer, length, f, &rmsFrames[f]);
    }

    // Smoothed envelope (running max with decay)
    float envelope = 0;
    uint32_t minGapFrames = (MIN_ONSET_GAP_MS * SAMPLE_RATE / 1000) / RMS_FRAME_SIZE;
    if (minGapFrames < 1) minGapFrames = 1;

    uint32_t lastOnsetFrame = 0;
    uint8_t sliceIdx = 0;

    for (uint32_t f = 1; f < numFrames; f++) {
        float rms = rmsFrames[f];

        // Update envelope
        if (rms > envelope) {
            envelope = rms;
        } else {
            envelope = envelope * ENVELOPE_ALPHA;
        }

        // Check for onset
        if (f - lastOnsetFrame >= minGapFrames) {
            if (rms > envelope / ONSET_THRESHOLD && rms > NOISE_FLOOR) {
                if (sliceIdx < MAX_SLICES) {
                    uint32_t samplePos = f * RMS_FRAME_SIZE;
                    result->slices[sliceIdx].onsetSample = samplePos;

                    if (sliceIdx > 0) {
                        uint32_t prevPos = result->slices[sliceIdx - 1].onsetSample;
                        result->slices[sliceIdx - 1].sliceLength = samplePos - prevPos;
                    }

                    sliceIdx++;
                    lastOnsetFrame = f;
                }
            }
        }
    }

    // Fix last slice length
    if (sliceIdx > 0) {
        result->slices[sliceIdx - 1].sliceLength =
            length - result->slices[sliceIdx - 1].onsetSample;
    }

    result->count = sliceIdx;
    result->detected = (sliceIdx >= MIN_SLICES);

    Serial.printf("TransientDetector: %u slices detected\n", sliceIdx);
}

void TransientDetector_gridFallback(uint32_t length, uint32_t barSamples,
                                     TransientResult* result) {
    memset(result, 0, sizeof(TransientResult));

    if (barSamples == 0) barSamples = SAMPLE_RATE;  // Default 1 second

    uint8_t count = 0;
    uint32_t pos = 0;

    while (pos < length && count < MAX_SLICES) {
        result->slices[count].onsetSample = pos;
        uint32_t sliceLen = barSamples;
        if (pos + sliceLen > length) {
            sliceLen = length - pos;
        }
        result->slices[count].sliceLength = sliceLen;

        pos += sliceLen;
        count++;
    }

    // Fix last slice
    if (count > 0) {
        uint32_t lastOnset = result->slices[count - 1].onsetSample;
        result->slices[count - 1].sliceLength = length - lastOnset;
    }

    result->count = count;
    result->detected = false;  // Grid fallback - not detected

    Serial.printf("TransientDetector: Grid fallback → %u slices\n", count);
}