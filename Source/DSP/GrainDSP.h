/*
  ==============================================================================

    GrainDSP.h
    Pure DSP functions for GRAIN saturation processor

  ==============================================================================
*/

#pragma once

#include <cmath>

namespace GrainDSP
{
//==============================================================================
// RMS Detector constants
constexpr float kRmsAttackMs = 100.0f;   // Slow attack (ignores transients)
constexpr float kRmsReleaseMs = 300.0f;  // Even slower release (stability)

//==============================================================================
// Dynamic Bias constants (Task 004)
constexpr float kBiasAmount = 0.3f;         // Internal bias intensity (future: from Grain param)
constexpr float kBiasScale = 0.1f;          // Keeps effect in micro-saturation territory
constexpr float kDCBlockerCutoffHz = 5.0f;  // DC blocker cutoff frequency

//==============================================================================
/**
 * Stateless helper for calculating one-pole filter coefficient.
 * @param sampleRate Sample rate in Hz
 * @param timeMs Time constant in milliseconds
 * @return Filter coefficient (0-1, higher = slower response)
 */
inline float calculateCoefficient(float sampleRate, float timeMs)
{
    return std::exp(-1.0f / (sampleRate * timeMs * 0.001f));
}

//==============================================================================
/**
 * Slow RMS level detector with asymmetric ballistics.
 * Provides a stable envelope that intentionally ignores transients.
 * Used by Dynamic Bias stage to modulate saturation character.
 */
struct RMSDetector
{
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    /**
     * Prepare the detector for a given sample rate.
     * @param sampleRate Sample rate in Hz
     * @param attackMs Attack time in milliseconds
     * @param releaseMs Release time in milliseconds
     */
    void prepare(float sampleRate, float attackMs, float releaseMs)
    {
        attackCoeff = calculateCoefficient(sampleRate, attackMs);
        releaseCoeff = calculateCoefficient(sampleRate, releaseMs);
    }

    /**
     * Reset the detector state (clears envelope history).
     */
    void reset() { envelope = 0.0f; }

    /**
     * Process a single sample and return the RMS envelope.
     * @param input Input sample
     * @return RMS envelope value (always >= 0)
     */
    float process(float input)
    {
        const float inputSquared = input * input;

        // Asymmetric ballistics: different attack/release
        const float coeff = (inputSquared > envelope) ? attackCoeff : releaseCoeff;

        // One-pole smoothing filter
        envelope = (envelope * coeff) + (inputSquared * (1.0f - coeff));

        // Return RMS (square root of mean square)
        return std::sqrt(envelope);
    }
};

//==============================================================================
/**
 * Apply dynamic bias for even-harmonic generation (Task 004).
 * Adds a quadratic term proportional to RMS level, breaking waveform symmetry.
 * @param input The input sample
 * @param rmsLevel Current RMS envelope value from detector
 * @param biasAmount Bias intensity (0.0 = no bias, 1.0 = full bias)
 * @return Biased output sample
 */
inline float applyDynamicBias(float input, float rmsLevel, float biasAmount)
{
    const float bias = rmsLevel * biasAmount * kBiasScale;
    return input + (bias * input * input);
}

//==============================================================================
/**
 * One-pole DC blocker (high-pass filter at ~5Hz).
 * Removes DC offset introduced by the quadratic bias term.
 * Transfer function: y[n] = x[n] - x[n-1] + coeff * y[n-1]
 */
struct DCBlocker
{
    float x1 = 0.0f;
    float y1 = 0.0f;
    float coeff = 0.9993f;

    /**
     * Prepare the DC blocker for a given sample rate.
     * @param sampleRate Sample rate in Hz
     */
    void prepare(float sampleRate)
    {
        constexpr float kTwoPi = 6.283185307f;
        coeff = 1.0f - (kTwoPi * kDCBlockerCutoffHz / sampleRate);
    }

    /**
     * Reset the DC blocker state.
     */
    void reset()
    {
        x1 = 0.0f;
        y1 = 0.0f;
    }

    /**
     * Process a single sample.
     * @param input Input sample
     * @return DC-blocked output sample
     */
    float process(float input)
    {
        const float output = (input - x1) + (coeff * y1);
        x1 = input;
        y1 = output;
        return output;
    }
};

/**
 * Apply tanh waveshaper with drive control.
 * @param input The input sample
 * @param drive Normalized drive amount (0.0 - 1.0), maps to 1x-4x gain
 * @return Saturated output sample (bounded to -1..+1)
 */
inline float applyWaveshaper(float input, float drive)
{
    const float gained = input * (1.0f + drive * 3.0f);  // 1x to 4x gain
    return std::tanh(gained);
}

/**
 * Apply dry/wet mix blend.
 * @param dry The dry (unprocessed) sample
 * @param wet The wet (processed) sample
 * @param mix Mix amount (0.0 = full dry, 1.0 = full wet)
 * @return Blended output sample
 */
inline float applyMix(float dry, float wet, float mix)
{
    return (wet * mix) + (dry * (1.0f - mix));
}

/**
 * Apply linear gain.
 * @param input The input sample
 * @param gainLinear Linear gain multiplier (1.0 = unity)
 * @return Gained output sample
 */
inline float applyGain(float input, float gainLinear)
{
    return input * gainLinear;
}
}  // namespace GrainDSP
