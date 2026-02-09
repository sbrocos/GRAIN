/*
  ==============================================================================

    Waveshaper.h
    tanh saturation with drive control

  ==============================================================================
*/

#pragma once

#include <cmath>

namespace GrainDSP
{
//==============================================================================
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

}  // namespace GrainDSP
