/**
 * Stage Manager Implementation
 *
 * Manages the 4-stage state machine: IDLE → CAPTURE_REVIEW → CHOP ↔ RESONATE
 * With wired parameter handlers for encoder/joystick.
 */

#include "stage_manager.h"
#include "../config.h"
#include "audio_engine.h"
#include "retroactive_buffer.h"
#include "bpm_clock.h"
#include "../audio/chop_engine.h"
#include <Arduino.h>

// ─── State ──────────────────────────────────────────────────────────────────

static Stage currentStage = STAGE_IDLE;
static Stage previousStage = STAGE_IDLE;
static uint32_t stageEnterMs = 0;

static const char* stageNames[] = {
    "IDLE", "CAPTURE", "CHOP", "RESONATE"
};

// Per-stage parameters
static int8_t pitchSemitones = 0;     // CAPTURE_REVIEW: ±12
static float chopDensity = 0.5f;      // CHOP: 0.0-1.0
static float wetDryMix = 0.0f;        // RESONATE: 0.0-1.0
static uint8_t barWindowIdx = 1;      // CAPTURE_REVIEW: index into {1,2,4,8}
static bool chopEnabled = false;
static bool resonatorEnabled = false;

static const uint8_t barWindows[] = {1, 2, 4, 8};
#define NUM_BAR_WINDOWS 4

// ─── Stage Transitions ──────────────────────────────────────────────────────

static void enterStage(Stage stage) {
    previousStage = currentStage;
    currentStage = stage;
    stageEnterMs = millis();

    Serial.printf("Stage: %s → %s\n", stageNames[previousStage], stageNames[stage]);

    switch (stage) {
        case STAGE_IDLE:
            RetroactiveBuffer_stopPlayback();
            RetroactiveBuffer_clear();
            AudioEngine_setPlaying(false);
            AudioEngine_setChopEnabled(false);
            AudioEngine_setResonatorEnabled(false);
            chopEnabled = false;
            resonatorEnabled = false;
            pitchSemitones = 0;
            chopDensity = 0.5f;
            wetDryMix = 0.0f;
            break;

        case STAGE_CAPTURE_REVIEW:
            AudioEngine_triggerCapture();
            RetroactiveBuffer_startPlayback();
            AudioEngine_setPlaying(true);
            pitchSemitones = 0;
            barWindowIdx = 1;
            break;

        case STAGE_CHOP:
            chopEnabled = true;
            AudioEngine_setChopEnabled(true);
            AudioEngine_setDensity(chopDensity);
            break;

        case STAGE_RESONATE:
            resonatorEnabled = true;
            AudioEngine_setResonatorEnabled(true);
            AudioEngine_setWetDry(wetDryMix);
            break;

        case STAGE_COUNT:
            break;
    }
}

// ─── Input Handlers ─────────────────────────────────────────────────────────

static void handleIdleInput(const InputMsg* msg) {
    if (msg->type == EVT_BUTTON_PRESS && msg->id == INPUT_BTN_1) {
        enterStage(STAGE_CAPTURE_REVIEW);
    }
}

static void handleCaptureInput(const InputMsg* msg) {
    switch (msg->type) {
        case EVT_JOY_RIGHT:
            enterStage(STAGE_CHOP);
            break;
        case EVT_JOY_LEFT:
            enterStage(STAGE_IDLE);
            break;
        case EVT_JOY_UP:
            if (barWindowIdx < NUM_BAR_WINDOWS - 1) {
                barWindowIdx++;
                Serial.printf("Bar window: %u bars\n", barWindows[barWindowIdx]);
            }
            break;
        case EVT_JOY_DOWN:
            if (barWindowIdx > 0) {
                barWindowIdx--;
                Serial.printf("Bar window: %u bars\n", barWindows[barWindowIdx]);
            }
            break;
        case EVT_ENCODER_CW:
            if (pitchSemitones < 12) {
                pitchSemitones++;
                AudioEngine_setPitch(pitchSemitones);
            }
            break;
        case EVT_ENCODER_CCW:
            if (pitchSemitones > -12) {
                pitchSemitones--;
                AudioEngine_setPitch(pitchSemitones);
            }
            break;
        case EVT_ENCODER_PUSH:
            enterStage(STAGE_CHOP);
            break;
        default:
            break;
    }
}

static void handleChopInput(const InputMsg* msg) {
    switch (msg->type) {
        case EVT_ENCODER_PUSH:
            // Tap-tempo in CHOP stage
            BPMClock_tap();
            break;
        case EVT_JOY_CENTER:
            ChopEngine_randomize();
            break;
        case EVT_JOY_RIGHT:
            enterStage(STAGE_RESONATE);
            break;
        case EVT_JOY_LEFT:
            enterStage(STAGE_CAPTURE_REVIEW);
            break;
        case EVT_ENCODER_CW:
            chopDensity += 0.05f;
            if (chopDensity > 1.0f) chopDensity = 1.0f;
            AudioEngine_setDensity(chopDensity);
            break;
        case EVT_ENCODER_CCW:
            chopDensity -= 0.05f;
            if (chopDensity < 0.0f) chopDensity = 0.0f;
            AudioEngine_setDensity(chopDensity);
            break;
        default:
            break;
    }
}

static void handleResonateInput(const InputMsg* msg) {
    switch (msg->type) {
        case EVT_ENCODER_PUSH:
            // Tap-tempo in RESONATE stage
            BPMClock_tap();
            break;
        case EVT_JOY_UP:
        case EVT_JOY_DOWN:
            // Chord select — placeholder until resonator is wired (Phase 6)
            break;
        case EVT_JOY_LEFT:
            enterStage(STAGE_CHOP);
            break;
        case EVT_ENCODER_CW:
            wetDryMix += 0.05f;
            if (wetDryMix > 1.0f) wetDryMix = 1.0f;
            AudioEngine_setWetDry(wetDryMix);
            break;
        case EVT_ENCODER_CCW:
            wetDryMix -= 0.05f;
            if (wetDryMix < 0.0f) wetDryMix = 0.0f;
            AudioEngine_setWetDry(wetDryMix);
            break;
        default:
            break;
    }
}

// ─── Public API ─────────────────────────────────────────────────────────────

void StageManager_init(void) {
    currentStage = STAGE_IDLE;
    previousStage = STAGE_IDLE;
    stageEnterMs = millis();
    Serial.println("StageManager: Initialized at IDLE");
}

Stage StageManager_current(void) {
    return currentStage;
}

const char* StageManager_stageName(void) {
    return stageNames[currentStage];
}

void StageManager_handleInput(const InputMsg* msg) {
    if (!msg || msg->type == EVT_NONE) return;

    // Shift+Capture → reset to IDLE from any stage
    if (msg->type == EVT_BUTTON_PRESS && msg->id == INPUT_BTN_1) {
        if (MCPInput_isShiftHeld()) {
            enterStage(STAGE_IDLE);
            return;
        }
    }

    switch (currentStage) {
        case STAGE_IDLE:
            handleIdleInput(msg);
            break;
        case STAGE_CAPTURE_REVIEW:
            handleCaptureInput(msg);
            break;
        case STAGE_CHOP:
            handleChopInput(msg);
            break;
        case STAGE_RESONATE:
            handleResonateInput(msg);
            break;
        case STAGE_COUNT:
            break;
    }
}

void StageManager_update(void) {
    // Per-tick updates (UI animation, etc.) — driven by UIManager
}

void StageManager_goto(Stage stage) {
    if (stage >= 0 && stage < STAGE_COUNT) {
        enterStage(stage);
    }
}
