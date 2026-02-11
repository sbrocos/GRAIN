/*
  ==============================================================================

    SpectralFocus.h
    Spectral focus module for GRAIN saturation processor (Task 006c).
    Gently biases where harmonic generation is emphasized using
    a low shelf (200 Hz) and high shelf (4 kHz) biquad pair.

    Each instance is mono — stereo is managed by creating two instances.
    Coefficients from Audio EQ Cookbook (Robert Bristow-Johnson).

  ==============================================================================
*/

#pragma once

#include "CalibrationConfig.h"

#include <cmath>

namespace GrainDSP
{
//==============================================================================
/**
 * Discrete spectral focus modes.
 * Each mode applies a complementary pair of shelf filters
 * to bias the harmonic generation region.
 */
enum class FocusMode
{
    Low = 0,  // Emphasis below 200 Hz (thicker, heavier bottom end)
    Mid = 1,  // Emphasis 200 Hz – 4 kHz (balanced presence)
    High = 2  // Emphasis above 4 kHz (airy, crisp top end)
};

//==============================================================================
/**
 * Spectral Focus using biquad shelf filters (Task 006c).
 * Mono module — create two instances for stereo processing.
 * Uses Transposed Direct Form II biquad implementation.
 */
struct SpectralFocus
{
    /**
     * Transposed Direct Form II biquad filter state.
     */
    struct BiquadState
    {
        float b0 = 1.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
        float z1 = 0.0f;
        float z2 = 0.0f;

        float process(float input)
        {
            const float output = (b0 * input) + z1;
            z1 = (b1 * input) - (a1 * output) + z2;
            z2 = (b2 * input) - (a2 * output);
            return output;
        }

        void reset()
        {
            z1 = 0.0f;
            z2 = 0.0f;
        }
    };

    // One filter pair per mono instance: low shelf + high shelf
    BiquadState lowShelf;
    BiquadState highShelf;

    /**
     * Prepare the filters for a given sample rate and focus mode.
     * Recalculates coefficients. Does NOT reset filter state
     * (call reset() explicitly if needed).
     * @param sampleRate Sample rate in Hz
     * @param mode Focus mode (Low, Mid, High)
     * @param cal Focus calibration parameters
     */
    void prepare(float sampleRate, FocusMode mode, const FocusCalibration& cal)
    {
        float lowGainDb = 0.0f;
        float highGainDb = 0.0f;

        switch (mode)
        {
            case FocusMode::Low:
                lowGainDb = cal.shelfGainDb;    // +3 dB
                highGainDb = -cal.shelfGainDb;  // -3 dB
                break;

            case FocusMode::Mid:
                lowGainDb = -cal.shelfGainDb * 0.5f;   // -1.5 dB
                highGainDb = -cal.shelfGainDb * 0.5f;  // -1.5 dB
                break;

            case FocusMode::High:
                lowGainDb = -cal.shelfGainDb;  // -3 dB
                highGainDb = cal.shelfGainDb;  // +3 dB
                break;
        }

        const auto lowCoeffs = calculateLowShelf(sampleRate, cal.lowShelfFreq, cal.shelfQ, lowGainDb);
        const auto highCoeffs = calculateHighShelf(sampleRate, cal.highShelfFreq, cal.shelfQ, highGainDb);

        lowShelf.b0 = lowCoeffs.b0;
        lowShelf.b1 = lowCoeffs.b1;
        lowShelf.b2 = lowCoeffs.b2;
        lowShelf.a1 = lowCoeffs.a1;
        lowShelf.a2 = lowCoeffs.a2;

        highShelf.b0 = highCoeffs.b0;
        highShelf.b1 = highCoeffs.b1;
        highShelf.b2 = highCoeffs.b2;
        highShelf.a1 = highCoeffs.a1;
        highShelf.a2 = highCoeffs.a2;
    }

    /**
     * Process a single sample through both shelf filters.
     * @param input Input sample
     * @return Filtered output sample
     */
    float process(float input)
    {
        float output = lowShelf.process(input);
        output = highShelf.process(output);
        return output;
    }

    /**
     * Reset all filter states (clears delay elements).
     */
    void reset()
    {
        lowShelf.reset();
        highShelf.reset();
    }

private:
    struct Coefficients
    {
        float b0, b1, b2, a1, a2;
    };

    /**
     * Calculate low shelf biquad coefficients.
     * Reference: Audio EQ Cookbook (Robert Bristow-Johnson)
     */
    static Coefficients calculateLowShelf(float sampleRate, float freq, float q, float gainDb)
    {
        constexpr float kTwoPi = 6.283185307f;
        const float kA = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = kTwoPi * freq / sampleRate;
        const float cosw0 = std::cos(w0);
        const float sinw0 = std::sin(w0);
        const float alpha = sinw0 / (2.0f * q);
        const float sqrtA = std::sqrt(kA);

        const float a0 = (kA + 1.0f) + ((kA - 1.0f) * cosw0) + (2.0f * sqrtA * alpha);

        Coefficients c{};
        c.b0 = (kA * ((kA + 1.0f) - ((kA - 1.0f) * cosw0) + (2.0f * sqrtA * alpha))) / a0;
        c.b1 = (2.0f * kA * ((kA - 1.0f) - ((kA + 1.0f) * cosw0))) / a0;
        c.b2 = (kA * ((kA + 1.0f) - ((kA - 1.0f) * cosw0) - (2.0f * sqrtA * alpha))) / a0;
        c.a1 = (-2.0f * ((kA - 1.0f) + ((kA + 1.0f) * cosw0))) / a0;
        c.a2 = ((kA + 1.0f) + ((kA - 1.0f) * cosw0) - (2.0f * sqrtA * alpha)) / a0;

        return c;
    }

    /**
     * Calculate high shelf biquad coefficients.
     * Reference: Audio EQ Cookbook (Robert Bristow-Johnson)
     */
    static Coefficients calculateHighShelf(float sampleRate, float freq, float q, float gainDb)
    {
        constexpr float kTwoPi = 6.283185307f;
        const float kA = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = kTwoPi * freq / sampleRate;
        const float cosw0 = std::cos(w0);
        const float sinw0 = std::sin(w0);
        const float alpha = sinw0 / (2.0f * q);
        const float sqrtA = std::sqrt(kA);

        const float a0 = (kA + 1.0f) - ((kA - 1.0f) * cosw0) + (2.0f * sqrtA * alpha);

        Coefficients c{};
        c.b0 = (kA * ((kA + 1.0f) + ((kA - 1.0f) * cosw0) + (2.0f * sqrtA * alpha))) / a0;
        c.b1 = (-2.0f * kA * ((kA - 1.0f) + ((kA + 1.0f) * cosw0))) / a0;
        c.b2 = (kA * ((kA + 1.0f) + ((kA - 1.0f) * cosw0) - (2.0f * sqrtA * alpha))) / a0;
        c.a1 = (2.0f * ((kA - 1.0f) - ((kA + 1.0f) * cosw0))) / a0;
        c.a2 = ((kA + 1.0f) - ((kA - 1.0f) * cosw0) - (2.0f * sqrtA * alpha)) / a0;

        return c;
    }
};

}  // namespace GrainDSP
