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
}
