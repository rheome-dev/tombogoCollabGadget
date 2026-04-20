/**
 * Effects Chain DSP Module - Stub Implementation
 */

#include "effects_chain.h"
#include <Arduino.h>
#include <string.h>

void EffectsChain_init(void) {
    Serial.println("Effects Chain: initialized (stub)");
}

void EffectsChain_process(const int16_t* input, int16_t* output, uint32_t samples) {
    // Stub: pass-through
    if (input && output) {
        memcpy(output, input, samples * sizeof(int16_t));
    }
}

void EffectsChain_setParams(EffectType effect, const EffectParams* params) {
    (void)effect;
    (void)params;
}

void EffectsChain_getParams(EffectType effect, EffectParams* params) {
    (void)effect;
    if (params) {
        params->wetDry = 0.0f;
        params->param1 = 0.0f;
        params->param2 = 0.0f;
        params->bypass = true;
    }
}

void EffectsChain_setBypass(EffectType effect, bool bypass) {
    (void)effect;
    (void)bypass;
}

void EffectsChain_setStepParams(uint8_t step, EffectType effect, const EffectParams* params) {
    (void)step;
    (void)effect;
    (void)params;
}

void EffectsChain_step(void) {
}

void EffectsChain_reset(void) {
}
