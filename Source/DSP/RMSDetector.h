/*
  ==============================================================================

    RMSDetector.h
    Slow RMS level detector with asymmetric ballistics

  ==============================================================================
*/

#pragma once

#include "DSPHelpers.h"

#include <cmath>

namespace GrainDSP
{
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

}  // namespace GrainDSP
