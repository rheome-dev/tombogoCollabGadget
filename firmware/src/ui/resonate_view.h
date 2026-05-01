/**
 * Resonate Stage View
 *
 * Cluster of 16 chord shapes filling the round 466x466 display:
 *   - Inner ring (7 circles): diatonic chords i, ii°, III, iv, v, VI, VII
 *   - Outer ring (9 squares): extended chords V7, i9, iv9, v9, VI9, III9,
 *                              VII9, i11, sus4
 *
 * Each shape has a unique hue (HSV rotation around chord index), lit up
 * brighter with a glow border when active. Tapping a shape requests a chord
 * change quantized to the next 8th note.
 */

#ifndef UI_RESONATE_VIEW_H
#define UI_RESONATE_VIEW_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Build the chord cluster on the active LVGL screen. Hidden by default. */
void ResonateView_create(void);

/** Show the cluster (call when entering STAGE_RESONATE). */
void ResonateView_show(void);

/** Hide the cluster (call when leaving STAGE_RESONATE). */
void ResonateView_hide(void);

/** Update which chord is highlighted (idx 0..15). */
void ResonateView_setActiveChord(uint8_t idx);

#ifdef __cplusplus
}
#endif

#endif // UI_RESONATE_VIEW_H
