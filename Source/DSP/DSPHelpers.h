/*
  ==============================================================================

    DSPHelpers.h
    Utility pure functions for GRAIN DSP pipeline

  ==============================================================================
*/

#pragma once

#include <cmath>

namespace GrainDSP
{
//==============================================================================
// Math constants (moved from Constants.h during Task 007b)
constexpr float kTwoPi = 6.283185307f;
constexpr float kPi = 3.141592653f;

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

//==============================================================================
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
