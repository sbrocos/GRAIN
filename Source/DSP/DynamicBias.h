/*
  ==============================================================================

    DynamicBias.h
    Level-dependent asymmetry for even-harmonic generation

  ==============================================================================
*/

#pragma once

#include "CalibrationConfig.h"

namespace GrainDSP
{
//==============================================================================
/**
 * Apply dynamic bias for even-harmonic generation (Task 004).
 * Adds a quadratic term proportional to RMS level, breaking waveform symmetry.
 * @param input The input sample
 * @param rmsLevel Current RMS envelope value from detector
 * @param biasAmount Bias intensity (0.0 = no bias, 1.0 = full bias)
 * @param cal Bias calibration parameters
 * @return Biased output sample
 */
inline float applyDynamicBias(float input, float rmsLevel, float biasAmount, const BiasCalibration& cal)
{
    const float bias = rmsLevel * biasAmount * cal.scale;
    return input + (bias * input * input);
}

}  // namespace GrainDSP
