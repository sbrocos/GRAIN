/*
  ==============================================================================

    GrainDSPPipeline.h
    Per-channel DSP pipeline orchestrator for GRAIN saturation processor.
    Each instance is mono — stereo is managed by creating two instances.

    Signal chain (with oversampling):
    [Upsample] → Dynamic Bias → Waveshaper → Warmth → Focus → [Downsample] → Mix → DC Blocker → Gain

  ==============================================================================
*/

#pragma once

#include "CalibrationConfig.h"
#include "DCBlocker.h"
#include "DSPHelpers.h"
#include "DynamicBias.h"
#include "SpectralFocus.h"
#include "WarmthProcessor.h"
#include "Waveshaper.h"

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
    SpectralFocus spectralFocus;

    /**
     * Prepare all stateful modules for a given sample rate.
     * @param sampleRate Sample rate in Hz
     * @param focusMode Initial spectral focus mode
     * @param cal Calibration config for all DSP modules
     */
    void prepare(float sampleRate, FocusMode focusMode, const CalibrationConfig& cal)
    {
        config = cal;
        dcBlocker.prepare(sampleRate, cal.dcBlocker);
        spectralFocus.prepare(sampleRate, focusMode, cal.focus);
    }

    /**
     * Update spectral focus coefficients for a new mode.
     * Does NOT reset filter state (avoids clicks on mode change).
     * @param sampleRate Sample rate in Hz
     * @param focusMode New spectral focus mode
     */
    void setFocusMode(float sampleRate, FocusMode focusMode)
    {
        spectralFocus.prepare(sampleRate, focusMode, config.focus);
    }

    /**
     * Reset all stateful module states.
     */
    void reset()
    {
        dcBlocker.reset();
        spectralFocus.reset();
    }

    /**
     * Process the nonlinear ("wet") stages of the DSP chain.
     * Runs at oversampled rate when oversampling is active.
     * @param input Input sample
     * @param envelope Current RMS envelope value from detector
     * @param drive Normalized drive amount (0.0 - 1.0)
     * @param warmth Warmth amount (0.0 = neutral, 1.0 = maximum warmth)
     * @return Wet (processed) output sample
     */
    float processWet(float input, float envelope, float drive, float warmth)
    {
        const float biased = applyDynamicBias(input, envelope, config.bias.amount, config.bias);
        const float shaped = applyWaveshaper(biased, drive);
        const float warmed = applyWarmth(shaped, warmth, config.warmth);
        return spectralFocus.process(warmed);
    }

    /**
     * Process the linear stages: dry/wet mix, DC blocker, output gain.
     * Runs at original sample rate (no need to oversample linear operations).
     * @param dry The original dry signal
     * @param wet The processed wet signal (from processWet, after downsampling)
     * @param mix Mix amount (0.0 = full dry, 1.0 = full wet)
     * @param gain Linear gain multiplier (1.0 = unity)
     * @return Final output sample
     */
    float processMixGain(float dry, float wet, float mix, float gain)
    {
        const float mixed = applyMix(dry, wet, mix);
        const float dcBlocked = dcBlocker.process(mixed);
        return applyGain(dcBlocked, gain);
    }

    /**
     * Process a single sample through the full DSP chain (convenience method).
     * Combines processWet + processMixGain. Used when oversampling is not active.
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
        const float wet = processWet(dry, envelope, drive, warmth);
        return processMixGain(dry, wet, mix, gain);
    }

private:
    CalibrationConfig config;
};

}  // namespace GrainDSP
