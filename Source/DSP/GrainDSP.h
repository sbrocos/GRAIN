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
constexpr float RMS_ATTACK_MS = 100.0f;   // Slow attack (ignores transients)
constexpr float RMS_RELEASE_MS = 300.0f;  // Even slower release (stability)

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
        float inputSquared = input * input;

        // Asymmetric ballistics: different attack/release
        float coeff = (inputSquared > envelope) ? attackCoeff : releaseCoeff;

        // One-pole smoothing filter
        envelope = envelope * coeff + inputSquared * (1.0f - coeff);

        // Return RMS (square root of mean square)
        return std::sqrt(envelope);
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
    float gained = input * (1.0f + drive * 3.0f);  // 1x to 4x gain
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
