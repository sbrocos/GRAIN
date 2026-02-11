/*
  ==============================================================================

    DCBlocker.h
    One-pole DC blocker (high-pass filter at ~5Hz)

  ==============================================================================
*/

#pragma once

#include "CalibrationConfig.h"

namespace GrainDSP
{
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
     * @param cal DC blocker calibration parameters
     */
    void prepare(float sampleRate, const DCBlockerCalibration& cal)
    {
        constexpr float kTwoPi = 6.283185307f;
        coeff = 1.0f - (kTwoPi * cal.cutoffHz / sampleRate);
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

}  // namespace GrainDSP
