/**
 * Transient Detector - Amplitude-based onset detection
 *
 * Detects slice boundaries in captured audio using RMS envelope tracking.
 * Simplified for ESP32 (no FFT, just amplitude analysis).
 */

#ifndef TRANSIENT_DETECTOR_H
#define TRANSIENT_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

// ─── Configuration ────────────────────────────────────────────────────────────

#define ONSET_THRESHOLD     1.5f   // RMS must exceed envelope by 1.5×
#define MIN_ONSET_GAP_MS    50     // Minimum 50ms between onsets
#define MAX_SLICES          32
#define MIN_SLICES          2

// ─── Result Structure ─────────────────────────────────────────────────────────

typedef struct {
    uint32_t onsetSample;   // Sample index in buffer
    uint32_t sliceLength;   // Length in samples
} SliceInfo;

typedef struct {
    SliceInfo slices[MAX_SLICES];
    uint8_t   count;         // Number of detected slices
    bool      detected;      // True if transient detection succeeded
} TransientResult;

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * Detect transients in a mono audio buffer.
 * Outputs sorted onset positions.
 *
 * @param buffer    Mono audio samples (int16_t)
 * @param length    Number of samples
 * @param result    Output structure for detected slices
 */
void TransientDetector_detect(const int16_t* buffer, uint32_t length,
                               TransientResult* result);

/**
 * Fallback: divide buffer evenly at BPM grid boundaries.
 * Used when transient detection finds < 2 slices.
 *
 * @param length        Buffer length in samples
 * @param barSamples    Samples per bar at current BPM
 * @param result        Output structure
 */
void TransientDetector_gridFallback(uint32_t length, uint32_t barSamples,
                                     TransientResult* result);

#endif // TRANSIENT_DETECTOR_H