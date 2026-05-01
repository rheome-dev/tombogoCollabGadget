/**
 * Resonate Stage View — 16 chord shapes filling the circular display.
 *
 * Inner ring (idx 0-6, diatonic): round, smaller circles
 * Outer ring (idx 7-15, extended): square (slightly rounded), slightly larger
 *
 * Touch dispatches to StageManager_requestChord() which queues the change
 * for the next 8th-note boundary.
 */

#include "resonate_view.h"
#include "../core/stage_manager.h"
#include "../config.h"

#include <Arduino.h>
#include <lvgl.h>
#include <math.h>

#define DISPLAY_SIZE      SCREEN_WIDTH    // 466 (square round display)
#define CENTER            (DISPLAY_SIZE / 2)

#define INNER_RING_R      75
#define OUTER_RING_R      170
#define INNER_DIAMETER    52
#define OUTER_SIDE        58
#define OUTER_RADIUS_PX   6   // slight corner round on the outer squares

// Chord cluster
static lv_obj_t* g_container = nullptr;
static lv_obj_t* g_chordObjs[16] = { nullptr };
static lv_color_t g_chordColors[16];        // base color per chord
static uint8_t g_activeChord = 0xFF;        // 0xFF = none yet

// ─── Color palette ──────────────────────────────────────────────────────────
//
// 16 hues evenly spaced around the wheel. HSV(h, s=0.85, v=0.9) → RGB.
// Computed at create() time so we don't repeat the math per draw.

static lv_color_t hsv_to_rgb(float h, float s, float v) {
    float c = v * s;
    float h6 = h * 6.0f;
    float x = c * (1.0f - fabsf(fmodf(h6, 2.0f) - 1.0f));
    float r = 0, g = 0, b = 0;
    if      (h6 < 1) { r = c; g = x; b = 0; }
    else if (h6 < 2) { r = x; g = c; b = 0; }
    else if (h6 < 3) { r = 0; g = c; b = x; }
    else if (h6 < 4) { r = 0; g = x; b = c; }
    else if (h6 < 5) { r = x; g = 0; b = c; }
    else             { r = c; g = 0; b = x; }
    float m = v - c;
    uint8_t R = (uint8_t)((r + m) * 255.0f);
    uint8_t G = (uint8_t)((g + m) * 255.0f);
    uint8_t B = (uint8_t)((b + m) * 255.0f);
    return lv_color_make(R, G, B);
}

static void compute_palette(void) {
    for (int i = 0; i < 16; i++) {
        float h = (float)i / 16.0f;
        g_chordColors[i] = hsv_to_rgb(h, 0.85f, 0.90f);
    }
}

// ─── Touch handler ──────────────────────────────────────────────────────────

static void chord_pressed_cb(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target(e);
    uint8_t idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(obj);
    if (idx > 15) return;
    StageManager_requestChord(idx);
}

// ─── Style helpers ──────────────────────────────────────────────────────────

static void style_chord(lv_obj_t* obj, uint8_t idx, bool isActive) {
    lv_color_t base = g_chordColors[idx];

    if (isActive) {
        // Bright fill, white outline + glow border
        lv_obj_set_style_bg_color(obj, base, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(obj, lv_color_white(), 0);
        lv_obj_set_style_border_width(obj, 3, 0);
        lv_obj_set_style_border_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_color(obj, base, 0);
        lv_obj_set_style_shadow_width(obj, 16, 0);
        lv_obj_set_style_shadow_opa(obj, LV_OPA_70, 0);
    } else {
        // Dim fill, no border, no glow
        lv_obj_set_style_bg_color(obj, base, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_set_style_shadow_width(obj, 0, 0);
    }
}

// ─── Layout ─────────────────────────────────────────────────────────────────

static void place_inner_ring(void) {
    // 7 circles around inner ring, evenly spaced. Top of ring = chord 0.
    for (int i = 0; i < 7; i++) {
        float angle = ((float)i / 7.0f) * (float)M_PI * 2.0f - (float)M_PI / 2.0f;
        int x = CENTER + (int)(cosf(angle) * INNER_RING_R) - INNER_DIAMETER / 2;
        int y = CENTER + (int)(sinf(angle) * INNER_RING_R) - INNER_DIAMETER / 2;

        lv_obj_t* obj = lv_obj_create(g_container);
        lv_obj_remove_style_all(obj);
        lv_obj_set_size(obj, INNER_DIAMETER, INNER_DIAMETER);
        lv_obj_set_pos(obj, x, y);
        lv_obj_set_style_radius(obj, INNER_DIAMETER / 2, 0);  // perfect circle
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(obj, (void*)(uintptr_t)i);
        lv_obj_add_event_cb(obj, chord_pressed_cb, LV_EVENT_PRESSED, nullptr);
        style_chord(obj, i, false);
        g_chordObjs[i] = obj;
    }
}

static void place_outer_ring(void) {
    // 9 squares around outer ring. Chord 7 starts at top.
    for (int i = 0; i < 9; i++) {
        uint8_t chordIdx = 7 + i;
        float angle = ((float)i / 9.0f) * (float)M_PI * 2.0f - (float)M_PI / 2.0f;
        int x = CENTER + (int)(cosf(angle) * OUTER_RING_R) - OUTER_SIDE / 2;
        int y = CENTER + (int)(sinf(angle) * OUTER_RING_R) - OUTER_SIDE / 2;

        lv_obj_t* obj = lv_obj_create(g_container);
        lv_obj_remove_style_all(obj);
        lv_obj_set_size(obj, OUTER_SIDE, OUTER_SIDE);
        lv_obj_set_pos(obj, x, y);
        lv_obj_set_style_radius(obj, OUTER_RADIUS_PX, 0);  // square w/ slight round
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(obj, (void*)(uintptr_t)chordIdx);
        lv_obj_add_event_cb(obj, chord_pressed_cb, LV_EVENT_PRESSED, nullptr);
        style_chord(obj, chordIdx, false);
        g_chordObjs[chordIdx] = obj;
    }
}

// ─── Public API ─────────────────────────────────────────────────────────────

void ResonateView_create(void) {
    if (g_container) return;

    compute_palette();

    lv_obj_t* scr = lv_scr_act();

    g_container = lv_obj_create(scr);
    lv_obj_remove_style_all(g_container);
    lv_obj_set_size(g_container, DISPLAY_SIZE, DISPLAY_SIZE);
    lv_obj_set_pos(g_container, 0, 0);
    lv_obj_set_style_bg_color(g_container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_container, LV_OPA_COVER, 0);
    lv_obj_clear_flag(g_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_container, LV_OBJ_FLAG_HIDDEN);   // start hidden

    place_inner_ring();
    place_outer_ring();

    Serial.println("ResonateView: created (16 chord shapes)");
}

void ResonateView_show(void) {
    if (!g_container) return;
    lv_obj_clear_flag(g_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_container);
}

void ResonateView_hide(void) {
    if (!g_container) return;
    lv_obj_add_flag(g_container, LV_OBJ_FLAG_HIDDEN);
}

void ResonateView_setActiveChord(uint8_t idx) {
    if (idx > 15) return;
    if (g_activeChord == idx) return;

    if (g_activeChord <= 15 && g_chordObjs[g_activeChord]) {
        style_chord(g_chordObjs[g_activeChord], g_activeChord, false);
    }
    if (g_chordObjs[idx]) {
        style_chord(g_chordObjs[idx], idx, true);
    }
    g_activeChord = idx;
}
