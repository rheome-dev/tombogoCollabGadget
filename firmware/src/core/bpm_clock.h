/**
 * BPM Clock - Sample-accurate timing for synchronized audio
 *
 * Emits events on beat divisions (sixteenth, eighth, beat, bar).
 * Tap-tempo support via encoder push.
 */

#ifndef BPM_CLOCK_H
#define BPM_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

// ─── Callbacks ───────────────────────────────────────────────────────────────

typedef void (*ClockCallback)(uint8_t bar, uint8_t beat, uint8_t sub);

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * Initialize the BPM clock.
 */
void BPMClock_init(uint16_t bpm);

/**
 * Start the clock.
 */
void BPMClock_start(void);

/**
 * Stop the clock.
 */
void BPMClock_stop(void);

/**
 * Set BPM (60-180 range).
 */
void BPMClock_setBPM(uint16_t bpm);

/**
 * Get current BPM.
 */
uint16_t BPMClock_getBPM(void);

/**
 * Call this once per sample in the audio callback.
 * Returns true if a sixteenth-note boundary was crossed.
 */
bool BPMClock_tick(void);

/**
 * Register callback for sixteenth-note events.
 */
void BPMClock_onSixteenth(ClockCallback cb);

/**
 * Register callback for bar (beat 1) events.
 */
void BPMClock_onBar(ClockCallback cb);

/**
 * Get current position.
 */
uint8_t  BPMClock_getCurrentBar(void);
uint8_t  BPMClock_getCurrentBeat(void);
uint8_t  BPMClock_getCurrentSub(void);

/**
 * Tap tempo - call on each encoder push tap.
 * BPM recomputed from last 4-8 tap intervals.
 */
void BPMClock_tap(void);

/**
 * Get current bar duration in seconds.
 */
float BPMClock_barDurationSec(void);

#endif // BPM_CLOCK_H