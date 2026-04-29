/**
 * BPM Clock Implementation
 *
 * Sample-accurate timing using sample counting.
 * At 16kHz with 120 BPM:
 *   - 16th note = 16000 * 60 / (120 * 4) = 2000 samples
 *   - bar = 16000 * 60 / 120 = 8000 samples
 */

#include "bpm_clock.h"
#include "../config.h"
#include <Arduino.h>

// ─── State ───────────────────────────────────────────────────────────────────

static uint16_t bpm = 120;
static bool running = false;
static uint32_t samplesPerSub = 0;    // samples per 16th note
static uint32_t sampleCounter = 0;    // counts toward next subdivision

// Position tracking
static uint8_t currentBar = 0;       // 0-7 (8 bars per phrase)
static uint8_t currentBeat = 0;      // 0-3 (4 beats per bar)
static uint8_t currentSub = 0;        // 0-15 (16 subdivisions per bar)

// Callbacks
static ClockCallback onSixteenth = nullptr;
static ClockCallback onBar = nullptr;

// Tap tempo
#define TAP_TIMEOUT_MS    2000    // Reset if gap > 2 seconds
#define TAP_MIN_INTERVALS  3       // Need at least 3 intervals to compute BPM
static uint32_t tapTimes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t  tapCount = 0;
static uint32_t lastTapMs = 0;
static bool tapActive = false;

// ─── Recompute Timing ─────────────────────────────────────────────────────────

static void recompute(void) {
    // samples per 16th note = SAMPLE_RATE * 60 / (bpm * 4)
    samplesPerSub = (uint32_t)SAMPLE_RATE * 60U / (bpm * 4U);
}

// ─── Public API ───────────────────────────────────────────────────────────────

void BPMClock_init(uint16_t initialBPM) {
    bpm = initialBPM > 0 ? initialBPM : 120;
    recompute();
    sampleCounter = 0;
    currentBar = 0;
    currentBeat = 0;
    currentSub = 0;
    running = false;

    tapCount = 0;
    lastTapMs = 0;
    tapActive = false;

    Serial.printf("BPMClock: Initialized at %u BPM (%u samples/sub)\n",
                  bpm, (unsigned int)samplesPerSub);
}

void BPMClock_start(void) {
    running = true;
    sampleCounter = 0;
}

void BPMClock_stop(void) {
    running = false;
}

void BPMClock_setBPM(uint16_t newBPM) {
    if (newBPM < 60) newBPM = 60;
    if (newBPM > 180) newBPM = 180;
    if (newBPM == bpm) return;

    bpm = newBPM;
    recompute();
    sampleCounter = 0;  // Reset phase on tempo change

    Serial.printf("BPMClock: BPM set to %u\n", bpm);
}

uint16_t BPMClock_getBPM(void) {
    return bpm;
}

bool BPMClock_tick(void) {
    if (!running) return false;

    sampleCounter++;

    if (sampleCounter >= samplesPerSub) {
        sampleCounter -= samplesPerSub;

        // Advance position
        currentSub++;
        if (currentSub >= 16) {
            currentSub = 0;
            currentBeat++;
            if (currentBeat >= 4) {
                currentBeat = 0;
                currentBar++;
                if (currentBar >= 8) currentBar = 0;

                // Bar callback (beat 1)
                if (onBar) onBar(currentBar, currentBeat, currentSub);
            }
        }

        // Sixteenth note callback
        if (onSixteenth) onSixteenth(currentBar, currentBeat, currentSub);

        return true;  // Subdivision crossed
    }

    return false;
}

void BPMClock_onSixteenth(ClockCallback cb) {
    onSixteenth = cb;
}

void BPMClock_onBar(ClockCallback cb) {
    onBar = cb;
}

uint8_t BPMClock_getCurrentBar(void) { return currentBar; }
uint8_t BPMClock_getCurrentBeat(void) { return currentBeat; }
uint8_t BPMClock_getCurrentSub(void) { return currentSub; }

void BPMClock_tap(void) {
    uint32_t now = millis();

    if (now - lastTapMs > TAP_TIMEOUT_MS) {
        // Timeout - reset tap sequence
        tapCount = 0;
        tapActive = false;
    }

    lastTapMs = now;

    // Shift tap history
    for (int i = 7; i > 0; i--) {
        tapTimes[i] = tapTimes[i - 1];
    }
    tapTimes[0] = now;
    tapCount++;
    tapActive = true;

    // Compute BPM if we have enough taps
    if (tapCount >= TAP_MIN_INTERVALS) {
        // Calculate intervals between consecutive taps
        uint32_t totalMs = 0;
        uint8_t intervals = 0;
        for (int i = 0; i < tapCount - 1; i++) {
            uint32_t interval = tapTimes[i] - tapTimes[i + 1];
            if (interval > 0 && interval < TAP_TIMEOUT_MS) {
                totalMs += interval;
                intervals++;
            }
        }

        if (intervals > 0) {
            float avgMs = (float)totalMs / intervals;
            if (avgMs > 0) {
                uint16_t newBPM = (uint16_t)(60000.0f / avgMs);
                BPMClock_setBPM(newBPM);
                Serial.printf("BPMClock: Tap tempo → %u BPM\n", bpm);
            }
        }
    }
}

float BPMClock_barDurationSec(void) {
    // bar duration = samples per bar / sample rate
    // samples per bar = samples per 16th * 16
    return (float)(samplesPerSub * 16U) / (float)SAMPLE_RATE;
}