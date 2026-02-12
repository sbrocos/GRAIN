/*
  ==============================================================================

    RMSDetector.h
    Slow RMS level detector with asymmetric ballistics

  ==============================================================================
*/

#pragma once

#include "CalibrationConfig.h"
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
     * @param cal RMS calibration parameters (attack/release times)
     */
    void prepare(float sampleRate, const RMSCalibration& cal)
    {
        attackCoeff = calculateCoefficient(sampleRate, cal.attackMs);
        releaseCoeff = calculateCoefficient(sampleRate, cal.releaseMs);
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
