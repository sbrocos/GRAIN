/*
  ==============================================================================

    WarmthProcessor.h
    Even/odd harmonic balance via half-wave rectification blend

  ==============================================================================
*/

#pragma once

#include "CalibrationConfig.h"

#include <cmath>

namespace GrainDSP
{
//==============================================================================
/**
 * Apply warmth: subtle even/odd harmonic balance (Task 005).
 * Blends between symmetric (odd harmonics) and asymmetric (even harmonics)
 * components using half-wave rectification blend.
 * @param input Signal after waveshaper
 * @param warmth Warmth amount (0.0 = neutral, 1.0 = maximum warmth)
 * @param cal Warmth calibration parameters
 * @return Harmonically shaped signal
 */
inline float applyWarmth(float input, float warmth, const WarmthCalibration& cal)
{
    const float depth = warmth * cal.depth;
    const float asymmetric = input * std::abs(input);
    return input + (depth * (asymmetric - input));
}

}  // namespace GrainDSP
