/**
 * Effects Chain DSP Module
 *
 * Infiltrator-style sequencable effects.
 * Each effect can be sequenced per-step in the pattern.
 * Written in Faust for portability.
 */

#ifndef DSP_EFFECTS_CHAIN_H
#define DSP_EFFECTS_CHAIN_H

#include <stdint.h>

/**
 * Effect types
 */
enum EffectType {
    EFFECT_FILTER,
    EFFECT_BITCRUSHER,
    EFFECT_DELAY,
    EFFECT_REVERB,
    EFFECT_DISTORTION,
    EFFECT_GRANULAR,
    EFFECT_COUNT
};

/**
 * Effect parameters
 */
struct EffectParams {
    float wetDry;      // 0.0 = dry, 1.0 = wet
    float param1;      // Effect-specific
    float param2;      // Effect-specific
    bool bypass;       // On/off
};

/**
 * Initialize effects chain
 */
void EffectsChain_init(void);

/**
 * Process audio through all effects
 */
void EffectsChain_process(const int16_t* input, int16_t* output, uint32_t samples);

/**
 * Set parameters for an effect
 */
void EffectsChain_setParams(EffectType effect, const EffectParams* params);

/**
 * Get parameters for an effect
 */
void EffectsChain_getParams(EffectType effect, EffectParams* params);

/**
 * Enable/bypass an effect
 */
void EffectsChain_setBypass(EffectType effect, bool bypass);

/**
 * Set effect parameters for a specific step (per-step sequencing)
 * @param step Step index (0-15)
 * @param effect Effect to modify
 * @param params Parameters for this step
 */
void EffectsChain_setStepParams(uint8_t step, EffectType effect, const EffectParams* params);

/**
 * Advance to next step (call from sequencer)
 */
void EffectsChain_step(void);

/**
 * Reset to step 0
 */
void EffectsChain_reset(void);

#endif // DSP_EFFECTS_CHAIN_H
