/*
  ==============================================================================

    GrainDSPPipeline.h
    Per-channel DSP pipeline orchestrator for GRAIN saturation processor.
    Each instance is mono — stereo is managed by creating two instances.

    Signal chain:
    Dynamic Bias → Waveshaper → Warmth → Mix → DC Blocker → Gain

  ==============================================================================
*/

#pragma once

#include "Constants.h"
#include "DCBlocker.h"
#include "DSPHelpers.h"
#include "DynamicBias.h"
#include "Waveshaper.h"
#include "WarmthProcessor.h"

namespace GrainDSP
{
//==============================================================================
/**
 * Per-channel DSP pipeline. Owns all stateful modules for one channel.
 * Create two instances (L/R) for stereo processing.
 */
struct DSPPipeline
{
    DCBlocker dcBlocker;

    /**
     * Prepare all stateful modules for a given sample rate.
     * @param sampleRate Sample rate in Hz
     */
    void prepare(float sampleRate)
    {
        dcBlocker.prepare(sampleRate);
    }

    /**
     * Reset all stateful module states.
     */
    void reset()
    {
        dcBlocker.reset();
    }

    /**
     * Process a single sample through the full DSP chain.
     * @param dry The dry (unprocessed) input sample
     * @param envelope Current RMS envelope value from detector
     * @param drive Normalized drive amount (0.0 - 1.0)
     * @param warmth Warmth amount (0.0 = neutral, 1.0 = maximum warmth)
     * @param mix Mix amount (0.0 = full dry, 1.0 = full wet)
     * @param gain Linear gain multiplier (1.0 = unity)
     * @return Processed output sample
     */
    float processSample(float dry, float envelope, float drive, float warmth, float mix, float gain)
    {
        const float biased = applyDynamicBias(dry, envelope, kBiasAmount);
        const float shaped = applyWaveshaper(biased, drive);
        const float warmed = applyWarmth(shaped, warmth);
        const float mixed = applyMix(dry, warmed, mix);
        const float dcBlocked = dcBlocker.process(mixed);
        return applyGain(dcBlocked, gain);
    }
};

}  // namespace GrainDSP
