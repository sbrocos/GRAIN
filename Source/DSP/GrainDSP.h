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
// Math constants
constexpr float kTwoPi = 6.283185307f;

//==============================================================================
// RMS Detector constants
constexpr float kRmsAttackMs = 100.0f;   // Slow attack (ignores transients)
constexpr float kRmsReleaseMs = 300.0f;  // Even slower release (stability)

//==============================================================================
// Dynamic Bias constants (Task 004)
constexpr float kBiasAmount = 0.3f;         // Internal bias intensity (future: from Grain param)
constexpr float kBiasScale = 0.1f;          // Keeps effect in micro-saturation territory
constexpr float kDCBlockerCutoffHz = 5.0f;  // DC blocker cutoff frequency

//==============================================================================
// Warmth constants (Task 005)
constexpr float kWarmthDepth = 0.22f;  // Maximum effect depth (22%) - calibrating via listening tests

//==============================================================================
// Spectral Focus constants (Task 006)
constexpr float kFocusLowShelfFreq = 200.0f;    // Hz
constexpr float kFocusHighShelfFreq = 4000.0f;  // Hz
constexpr float kFocusShelfGainDb = 2.0f;       // dB (max boost/cut)
constexpr float kFocusShelfQ = 0.707f;          // Butterworth Q

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
        const float inputSquared = input * input;

        // Asymmetric ballistics: different attack/release
        const float coeff = (inputSquared > envelope) ? attackCoeff : releaseCoeff;

        // One-pole smoothing filter
        envelope = (envelope * coeff) + (inputSquared * (1.0f - coeff));

        // Return RMS (square root of mean square)
        return std::sqrt(envelope);
    }
};

//==============================================================================
/**
 * Apply dynamic bias for even-harmonic generation (Task 004).
 * Adds a quadratic term proportional to RMS level, breaking waveform symmetry.
 * @param input The input sample
 * @param rmsLevel Current RMS envelope value from detector
 * @param biasAmount Bias intensity (0.0 = no bias, 1.0 = full bias)
 * @return Biased output sample
 */
inline float applyDynamicBias(float input, float rmsLevel, float biasAmount)
{
    const float bias = rmsLevel * biasAmount * kBiasScale;
    return input + (bias * input * input);
}

//==============================================================================
/**
 * Apply warmth: subtle even/odd harmonic balance (Task 005).
 * Blends between symmetric (odd harmonics) and asymmetric (even harmonics)
 * components using half-wave rectification blend.
 * @param input Signal after waveshaper
 * @param warmth Warmth amount (0.0 = neutral, 1.0 = maximum warmth)
 * @return Harmonically shaped signal
 */
inline float applyWarmth(float input, float warmth)
{
    const float depth = warmth * kWarmthDepth;
    const float asymmetric = input * std::abs(input);
    return input + (depth * (asymmetric - input));
}

//==============================================================================
// Spectral Focus: discrete Low/Mid/High mode (Task 006)

enum class FocusMode
{
    Low = 0,
    Mid = 1,
    High = 2
};

/**
 * Spectral Focus using biquad shelf filters (Task 006).
 * Gently biases where harmonic generation is emphasized using
 * a low shelf (200 Hz) and high shelf (4 kHz) pair.
 * Coefficients from Audio EQ Cookbook (Robert Bristow-Johnson).
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

    // Two filters per channel: low shelf + high shelf
    BiquadState lowShelf[2];
    BiquadState highShelf[2];

    /**
     * Prepare the filters for a given sample rate and focus mode.
     * @param sampleRate Sample rate in Hz
     * @param mode Focus mode (Low, Mid, High)
     */
    void prepare(float sampleRate, FocusMode mode)
    {
        float lowGainDb = 0.0f;
        float highGainDb = 0.0f;

        switch (mode)
        {
            case FocusMode::Low:
                lowGainDb = kFocusShelfGainDb;    // +2 dB
                highGainDb = -kFocusShelfGainDb;  // -2 dB
                break;

            case FocusMode::Mid:
                lowGainDb = -kFocusShelfGainDb * 0.5f;   // -1 dB
                highGainDb = -kFocusShelfGainDb * 0.5f;  // -1 dB
                break;

            case FocusMode::High:
                lowGainDb = -kFocusShelfGainDb;  // -2 dB
                highGainDb = kFocusShelfGainDb;  // +2 dB
                break;
        }

        const auto lowCoeffs = calculateLowShelf(sampleRate, kFocusLowShelfFreq, kFocusShelfQ, lowGainDb);
        const auto highCoeffs = calculateHighShelf(sampleRate, kFocusHighShelfFreq, kFocusShelfQ, highGainDb);

        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].b0 = lowCoeffs.b0;
            lowShelf[ch].b1 = lowCoeffs.b1;
            lowShelf[ch].b2 = lowCoeffs.b2;
            lowShelf[ch].a1 = lowCoeffs.a1;
            lowShelf[ch].a2 = lowCoeffs.a2;

            highShelf[ch].b0 = highCoeffs.b0;
            highShelf[ch].b1 = highCoeffs.b1;
            highShelf[ch].b2 = highCoeffs.b2;
            highShelf[ch].a1 = highCoeffs.a1;
            highShelf[ch].a2 = highCoeffs.a2;
        }
    }

    /**
     * Process a single sample through both shelf filters.
     * @param input Input sample
     * @param channel Channel index (0 = left, 1 = right)
     * @return Filtered output sample
     */
    float process(float input, int channel)
    {
        float output = lowShelf[channel].process(input);
        output = highShelf[channel].process(output);
        return output;
    }

    /**
     * Reset all filter states.
     */
    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].reset();
            highShelf[ch].reset();
        }
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
     */
    void prepare(float sampleRate) { coeff = 1.0f - (kTwoPi * kDCBlockerCutoffHz / sampleRate); }

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
//==============================================================================
/**
 * Process a single sample through the full DSP chain:
 * Dynamic Bias → Waveshaper → Warmth → Focus → Mix → DC Blocker → Gain.
 * Eliminates per-channel code duplication in processBlock.
 * @param dry The dry (unprocessed) input sample
 * @param envelope Current RMS envelope value from detector
 * @param drive Normalized drive amount (0.0 - 1.0)
 * @param warmth Warmth amount (0.0 = neutral, 1.0 = maximum warmth)
 * @param mix Mix amount (0.0 = full dry, 1.0 = full wet)
 * @param gain Linear gain multiplier (1.0 = unity)
 * @param dcBlocker DC blocker instance for this channel (stateful)
 * @param focus Spectral focus instance (stateful)
 * @param channel Channel index (0 = left, 1 = right)
 * @return Processed output sample
 */
inline float processSample(float dry, float envelope, float drive, float warmth, float mix, float gain,
                           DCBlocker& dcBlocker, SpectralFocus& focus, int channel)
{
    const float biased = applyDynamicBias(dry, envelope, kBiasAmount);
    const float shaped = applyWaveshaper(biased, drive);
    const float warmed = applyWarmth(shaped, warmth);
    const float focused = focus.process(warmed, channel);
    const float mixed = applyMix(dry, focused, mix);
    const float dcBlocked = dcBlocker.process(mixed);
    return applyGain(dcBlocked, gain);
}
}  // namespace GrainDSP
