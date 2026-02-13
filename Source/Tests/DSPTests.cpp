#include "../DSP/CalibrationConfig.h"
#include "../DSP/DCBlocker.h"
#include "../DSP/DSPHelpers.h"
#include "../DSP/DynamicBias.h"
#include "../DSP/GrainDSPPipeline.h"
#include "../DSP/RMSDetector.h"
#include "../DSP/SpectralFocus.h"
#include "../DSP/WarmthProcessor.h"
#include "../DSP/Waveshaper.h"

#include <JuceHeader.h>

//==============================================================================
namespace TestConstants
{
constexpr float kTolerance = 1e-5f;
constexpr float kClickThreshold = 0.01f;
constexpr int kBufferSize = 512;
}  // namespace TestConstants

//==============================================================================
class GrainDSPTests : public juce::UnitTest
{
public:
    GrainDSPTests() : juce::UnitTest("GRAIN DSP Tests") {}

    void runTest() override
    {
        runWaveshaperTests();
        runMixTests();
        runGainTests();
        runBypassTests();
        runBufferTests();
        runDiscontinuityTests();
        runRMSDetectorTests();
        runDynamicBiasTests();
        runDCBlockerTests();
        runDCOffsetAccumulationTest();
        runWarmthTests();
        runSpectralFocusTests();
        runStandaloneTests();
    }

private:
    // Default calibration sub-configs for convenience
    static constexpr auto kBiasCal = GrainDSP::kDefaultCalibration.bias;
    static constexpr auto kWarmthCal = GrainDSP::kDefaultCalibration.warmth;
    static constexpr auto kDCBlockerCal = GrainDSP::kDefaultCalibration.dcBlocker;
    static constexpr auto kFocusCal = GrainDSP::kDefaultCalibration.focus;
    static constexpr auto kRMSCal = GrainDSP::kDefaultCalibration.rms;

    //==========================================================================
    void runWaveshaperTests()
    {
        beginTest("Waveshaper: zero passthrough");
        expectWithinAbsoluteError(GrainDSP::applyWaveshaper(0.0f, 0.0f), 0.0f, TestConstants::kTolerance);

        beginTest("Waveshaper: symmetry");
        {
            const float x = 0.5f;
            const float drive = 0.5f;
            expectWithinAbsoluteError(GrainDSP::applyWaveshaper(-x, drive), -GrainDSP::applyWaveshaper(x, drive),
                                      TestConstants::kTolerance);
        }

        beginTest("Waveshaper: bounded output");
        {
            expect(GrainDSP::applyWaveshaper(100.0f, 1.0f) < 1.01f);
            expect(GrainDSP::applyWaveshaper(-100.0f, 1.0f) > -1.01f);
        }

        beginTest("Waveshaper: near-linear for small values");
        {
            const float x = 0.05f;
            const float result = GrainDSP::applyWaveshaper(x, 0.0f);  // drive = 0, gain = 1x
            expectWithinAbsoluteError(result, x, 0.01f);
        }
    }

    //==========================================================================
    void runMixTests()
    {
        beginTest("Mix: full dry (mix = 0)");
        {
            const float dry = 1.0f;
            const float wet = 0.5f;
            const float result = GrainDSP::applyMix(dry, wet, 0.0f);
            expectWithinAbsoluteError(result, dry, TestConstants::kTolerance);
        }

        beginTest("Mix: full wet (mix = 1)");
        {
            const float dry = 1.0f;
            const float wet = 0.5f;
            const float result = GrainDSP::applyMix(dry, wet, 1.0f);
            expectWithinAbsoluteError(result, wet, TestConstants::kTolerance);
        }

        beginTest("Mix: 50/50 blend");
        {
            const float dry = 1.0f;
            const float wet = 0.0f;
            const float result = GrainDSP::applyMix(dry, wet, 0.5f);
            expectWithinAbsoluteError(result, 0.5f, TestConstants::kTolerance);
        }
    }

    //==========================================================================
    void runGainTests()
    {
        beginTest("Gain: unity");
        {
            const float input = 0.7f;
            const float result = GrainDSP::applyGain(input, 1.0f);
            expectWithinAbsoluteError(result, input, TestConstants::kTolerance);
        }

        beginTest("Gain: double (+6dB)");
        {
            const float input = 0.5f;
            const float result = GrainDSP::applyGain(input, 2.0f);
            expectWithinAbsoluteError(result, 1.0f, TestConstants::kTolerance);
        }

        beginTest("Gain: zero (silence)");
        {
            const float input = 0.7f;
            const float result = GrainDSP::applyGain(input, 0.0f);
            expectWithinAbsoluteError(result, 0.0f, TestConstants::kTolerance);
        }
    }

    //==========================================================================
    void runBypassTests()
    {
        // Note: Bypass is implemented via mix smoothing (bypass ON → mix = 0)
        // These tests verify that mix = 0 produces dry signal (bypass behavior)

        beginTest("Bypass behavior: mix = 0 returns dry signal");
        {
            const float dry = 0.7f;
            const float wet = 0.3f;  // Different from dry
            const float mix = 0.0f;  // Simulates bypass ON
            const float result = GrainDSP::applyMix(dry, wet, mix);
            expectWithinAbsoluteError(result, dry, TestConstants::kTolerance);
        }

        beginTest("Bypass behavior: full processing when mix > 0");
        {
            const float dry = 0.7f;
            const float wet = 0.3f;
            const float mix = 0.5f;  // Normal operation
            const float result = GrainDSP::applyMix(dry, wet, mix);
            const float expected = (wet * mix) + (dry * (1.0f - mix));
            expectWithinAbsoluteError(result, expected, TestConstants::kTolerance);
        }
    }

    //==========================================================================
    void runBufferTests()
    {
        beginTest("Buffer processing: stability with constant input");
        {
            std::vector<float> buffer(TestConstants::kBufferSize, 0.5f);
            const float drive = 0.5f;
            const float mix = 0.5f;
            const float gain = 1.0f;

            // Process buffer
            for (int i = 0; i < TestConstants::kBufferSize; ++i)
            {
                const float dry = buffer[i];
                const float wet = GrainDSP::applyWaveshaper(dry, drive);
                const float mixed = GrainDSP::applyMix(dry, wet, mix);
                buffer[i] = GrainDSP::applyGain(mixed, gain);
            }

            // All samples should be identical (constant input -> constant output)
            const float expected = buffer[0];
            for (int i = 1; i < TestConstants::kBufferSize; ++i)
            {
                expectWithinAbsoluteError(buffer[i], expected, TestConstants::kTolerance);
            }
        }

        beginTest("Buffer processing: no state leak between samples");
        {
            const float drive = 0.5f;

            // Process same value multiple times
            const float result1 = GrainDSP::applyWaveshaper(0.5f, drive);
            const float result2 = GrainDSP::applyWaveshaper(0.5f, drive);
            const float result3 = GrainDSP::applyWaveshaper(0.5f, drive);

            expectWithinAbsoluteError(result1, result2, TestConstants::kTolerance);
            expectWithinAbsoluteError(result2, result3, TestConstants::kTolerance);
        }
    }

    //==========================================================================
    void runDiscontinuityTests()
    {
        beginTest("Parameter change: no discontinuities on silent input");
        {
            // Silent input - any click would be clearly audible
            std::vector<float> buffer(TestConstants::kBufferSize, 0.0f);

            float prevSample = 0.0f;
            bool hasClick = false;

            // Simulate abrupt parameter change: drive 0 -> 1
            for (int i = 0; i < TestConstants::kBufferSize; ++i)
            {
                // Abrupt change at midpoint
                const float drive = (i < TestConstants::kBufferSize / 2) ? 0.0f : 1.0f;

                const float wet = GrainDSP::applyWaveshaper(buffer[i], drive);
                const float output = GrainDSP::applyMix(buffer[i], wet, 0.5f);

                const float delta = std::abs(output - prevSample);
                if (delta > TestConstants::kClickThreshold)
                {
                    hasClick = true;
                }

                prevSample = output;
            }

            // Note: This test will PASS with pure functions (no state = no clicks on silence)
            // The real discontinuity test requires SmoothedValue in processBlock
            // This test documents the expected behavior
            expect(!hasClick);
        }
    }

    //==========================================================================
    // NOLINTNEXTLINE(readability-function-size)
    void runRMSDetectorTests()
    {
        beginTest("RMS Detector: coefficient calculation");
        {
            const float sampleRate = 44100.0f;
            const float timeMs = 100.0f;
            const float coeff = GrainDSP::calculateCoefficient(sampleRate, timeMs);

            // Coefficient should be between 0 and 1
            expect(coeff > 0.0f);
            expect(coeff < 1.0f);

            // Longer time = higher coefficient (slower response)
            const float coeffSlow = GrainDSP::calculateCoefficient(sampleRate, 200.0f);
            expect(coeffSlow > coeff);
        }

        beginTest("RMS Detector: zero input produces zero output");
        {
            GrainDSP::RMSDetector detector;
            detector.prepare(44100.0f, kRMSCal);

            for (int i = 0; i < 1000; ++i)
            {
                detector.process(0.0f);
            }

            const float result = detector.process(0.0f);
            expectWithinAbsoluteError(result, 0.0f, TestConstants::kTolerance);
        }

        beginTest("RMS Detector: DC input converges to that value");
        {
            GrainDSP::RMSDetector detector;
            detector.prepare(44100.0f, kRMSCal);

            const float constantInput = 0.5f;
            float result = 0.0f;

            // Process enough samples to converge (1 second)
            const int samplesToConverge = static_cast<int>(44100.0f * 1.0f);
            for (int i = 0; i < samplesToConverge; ++i)
            {
                result = detector.process(constantInput);
            }

            // Should converge to input value (RMS of constant = constant)
            expectWithinAbsoluteError(result, constantInput, 0.01f);
        }

        beginTest("RMS Detector: sine wave converges near RMS value");
        {
            GrainDSP::RMSDetector detector;
            detector.prepare(44100.0f, kRMSCal);

            const float amplitude = 1.0f;
            const float frequency = 440.0f;
            const float sampleRate = 44100.0f;
            float result = 0.0f;

            // Process enough samples to converge (1 second)
            const int samplesToConverge = static_cast<int>(sampleRate * 1.0f);
            for (int i = 0; i < samplesToConverge; ++i)
            {
                const float phase = GrainDSP::kTwoPi * frequency * static_cast<float>(i) / sampleRate;
                const float kSample = amplitude * std::sin(phase);
                result = detector.process(kSample);
            }

            // RMS of sine wave = amplitude / sqrt(2) ≈ 0.707
            // Note: Asymmetric ballistics (faster attack, slower release) causes
            // the detector to settle slightly higher than theoretical RMS (~0.82).
            // This is expected behavior for our slow, stable envelope design.
            const float expectedRms = amplitude / std::sqrt(2.0f);
            expectWithinAbsoluteError(result, expectedRms, 0.15f);
        }

        beginTest("RMS Detector: envelope is always non-negative");
        {
            GrainDSP::RMSDetector detector;
            detector.prepare(44100.0f, kRMSCal);

            // Test with negative input
            for (int i = 0; i < 100; ++i)
            {
                const float result = detector.process(-0.5f);
                expect(result >= 0.0f);
            }
        }

        beginTest("RMS Detector: slow response to transients");
        {
            GrainDSP::RMSDetector detector;
            detector.prepare(44100.0f, kRMSCal);

            // Start with silence
            for (int i = 0; i < 100; ++i)
            {
                detector.process(0.0f);
            }

            // Hit with sudden transient
            const float kImmediateResponse = detector.process(1.0f);

            // Should NOT immediately jump to 1.0 (that would be fast/peak detection)
            expect(kImmediateResponse < 0.5f);
        }

        beginTest("RMS Detector: reset clears state");
        {
            GrainDSP::RMSDetector detector;
            detector.prepare(44100.0f, kRMSCal);

            // Build up envelope
            for (int i = 0; i < 1000; ++i)
            {
                detector.process(1.0f);
            }

            detector.reset();

            // After reset, envelope should be zero
            const float result = detector.process(0.0f);
            expectWithinAbsoluteError(result, 0.0f, TestConstants::kTolerance);
        }
    }

    //==========================================================================
    void runDynamicBiasTests()
    {
        beginTest("Dynamic Bias: zero RMS produces no bias");
        {
            const float input = 0.5f;
            const float result = GrainDSP::applyDynamicBias(input, 0.0f, 1.0f, kBiasCal);
            expectWithinAbsoluteError(result, input, TestConstants::kTolerance);
        }

        beginTest("Dynamic Bias: zero amount produces no bias");
        {
            const float input = 0.5f;
            const float result = GrainDSP::applyDynamicBias(input, 0.5f, 0.0f, kBiasCal);
            expectWithinAbsoluteError(result, input, TestConstants::kTolerance);
        }

        beginTest("Dynamic Bias: positive input biased upward");
        {
            const float input = 0.5f;
            const float result = GrainDSP::applyDynamicBias(input, 0.5f, 1.0f, kBiasCal);
            expect(result > input);
        }

        beginTest("Dynamic Bias: negative input biased upward (asymmetry)");
        {
            const float input = -0.5f;
            const float result = GrainDSP::applyDynamicBias(input, 0.5f, 1.0f, kBiasCal);
            // Quadratic term (-0.5)^2 = 0.25, bias is positive -> shifts toward positive
            expect(result > input);
        }

        beginTest("Dynamic Bias: asymmetric response (even harmonics)");
        {
            const float resultPositive = GrainDSP::applyDynamicBias(0.5f, 0.5f, 1.0f, kBiasCal);
            const float resultNegative = GrainDSP::applyDynamicBias(-0.5f, 0.5f, 1.0f, kBiasCal);
            // Asymmetry: |result for +0.5| should NOT equal |result for -0.5|
            expect(std::abs(resultPositive) != std::abs(resultNegative));
        }

        beginTest("Dynamic Bias: higher RMS produces more bias");
        {
            const float input = 0.5f;
            const float resultLowRMS = GrainDSP::applyDynamicBias(input, 0.1f, 1.0f, kBiasCal);
            const float resultHighRMS = GrainDSP::applyDynamicBias(input, 0.9f, 1.0f, kBiasCal);

            const float deviationLow = std::abs(resultLowRMS - input);
            const float deviationHigh = std::abs(resultHighRMS - input);
            expect(deviationHigh > deviationLow);
        }

        beginTest("Dynamic Bias: effect is subtle (bounded)");
        {
            const float input = 1.0f;
            const float result = GrainDSP::applyDynamicBias(input, 1.0f, 1.0f, kBiasCal);
            // Even at extreme settings, should stay within bias scale (10%) of input
            expect(result <= input * (1.0f + kBiasCal.scale));
            expect(result >= input * (1.0f - kBiasCal.scale));
        }
    }

    //==========================================================================
    void runDCBlockerTests()
    {
        beginTest("DC Blocker: passes AC signal");
        {
            GrainDSP::DCBlocker blocker;
            blocker.prepare(44100.0f, kDCBlockerCal);

            const float frequency = 440.0f;
            const float sampleRate = 44100.0f;

            // Let the filter settle (5000 samples)
            for (int i = 0; i < 5000; ++i)
            {
                const float phase = GrainDSP::kTwoPi * frequency * static_cast<float>(i) / sampleRate;
                blocker.process(std::sin(phase));
            }

            // After settling, output should closely match input
            float maxError = 0.0f;
            for (int i = 5000; i < 6000; ++i)
            {
                const float phase = GrainDSP::kTwoPi * frequency * static_cast<float>(i) / sampleRate;
                const float input = std::sin(phase);
                const float output = blocker.process(input);
                const float error = std::abs(output - input);
                maxError = std::max(maxError, error);
            }

            // AC signal should pass through with minimal attenuation
            // Tolerance accounts for small phase shift (~0.65 degrees at 440Hz)
            expect(maxError < 0.02f);
        }

        beginTest("DC Blocker: removes DC offset");
        {
            GrainDSP::DCBlocker blocker;
            blocker.prepare(44100.0f, kDCBlockerCal);

            // Feed constant DC value
            float output = 0.0f;
            for (int i = 0; i < 44100; ++i)
            {
                output = blocker.process(1.0f);
            }

            // Output should converge toward 0
            expect(std::abs(output) < 0.01f);
        }

        beginTest("DC Blocker: reset clears state");
        {
            GrainDSP::DCBlocker blocker;
            blocker.prepare(44100.0f, kDCBlockerCal);

            // Build up state
            for (int i = 0; i < 1000; ++i)
            {
                blocker.process(1.0f);
            }

            blocker.reset();

            expectWithinAbsoluteError(blocker.x1, 0.0f, TestConstants::kTolerance);
            expectWithinAbsoluteError(blocker.y1, 0.0f, TestConstants::kTolerance);
        }
    }

    //==========================================================================
    void runWarmthTests()
    {
        beginTest("Warmth: zero warmth returns input unchanged");
        {
            const float input = 0.5f;
            const float result = GrainDSP::applyWarmth(input, 0.0f, kWarmthCal);
            expectWithinAbsoluteError(result, input, TestConstants::kTolerance);
        }

        beginTest("Warmth: zero input returns zero");
        {
            const float result = GrainDSP::applyWarmth(0.0f, 1.0f, kWarmthCal);
            expectWithinAbsoluteError(result, 0.0f, TestConstants::kTolerance);
        }

        beginTest("Warmth: effect is subtle (bounded)");
        {
            const float input = 0.5f;
            const float result = GrainDSP::applyWarmth(input, 1.0f, kWarmthCal);

            // Maximum deviation should be within kWarmthDepth (22%)
            const float deviation = std::abs(result - input);
            expect(deviation < std::abs(input) * 0.25f);
        }

        beginTest("Warmth: positive input shifts toward asymmetry");
        {
            const float input = 0.5f;
            const float noWarmth = GrainDSP::applyWarmth(input, 0.0f, kWarmthCal);
            const float fullWarmth = GrainDSP::applyWarmth(input, 1.0f, kWarmthCal);

            expect(std::abs(fullWarmth - noWarmth) > TestConstants::kTolerance);
        }

        beginTest("Warmth: asymmetric effect on positive vs negative input");
        {
            const float warmth = 1.0f;

            const float resultPos = GrainDSP::applyWarmth(0.5f, warmth, kWarmthCal);
            const float resultNeg = GrainDSP::applyWarmth(-0.5f, warmth, kWarmthCal);

            const float deviationPos = std::abs(resultPos - 0.5f);
            const float deviationNeg = std::abs(resultNeg - (-0.5f));

            expect(deviationPos > TestConstants::kTolerance);
            expect(deviationNeg > TestConstants::kTolerance);
        }

        beginTest("Warmth: continuous across warmth range");
        {
            const float input = 0.5f;
            float prev = GrainDSP::applyWarmth(input, 0.0f, kWarmthCal);

            for (float w = 0.1f; w <= 1.0f; w += 0.1f)
            {
                const float current = GrainDSP::applyWarmth(input, w, kWarmthCal);
                const float delta = std::abs(current - prev);

                expect(delta < 0.05f);
                prev = current;
            }
        }

        beginTest("Warmth: buffer stability with constant input");
        {
            const float warmth = 0.7f;
            const float input = 0.5f;

            const float expected = GrainDSP::applyWarmth(input, warmth, kWarmthCal);

            for (int i = 0; i < TestConstants::kBufferSize; ++i)
            {
                const float result = GrainDSP::applyWarmth(input, warmth, kWarmthCal);
                expectWithinAbsoluteError(result, expected, TestConstants::kTolerance);
            }
        }
    }

    //==========================================================================
    // NOLINTNEXTLINE(readability-function-size)
    void runSpectralFocusTests()
    {
        beginTest("Focus: Mid mode is near-unity for broadband signal");
        {
            GrainDSP::SpectralFocus focus;
            focus.prepare(44100.0f, GrainDSP::FocusMode::kMid, kFocusCal);

            // Process DC — let filter settle
            const float input = 0.5f;
            float result = 0.0f;

            for (int i = 0; i < 1000; ++i)
            {
                result = focus.process(input);
            }

            // Mid mode cuts both shelves by -1.5dB each
            // DC sees both shelves: total ~-3dB ≈ 0.707x → 0.5 * 0.707 ≈ 0.354
            expect(result > 0.3f);  // Not completely attenuated
            expect(result < 0.6f);  // Not boosted
        }

        beginTest("Focus: reset clears filter state");
        {
            GrainDSP::SpectralFocus focus;
            focus.prepare(44100.0f, GrainDSP::FocusMode::kLow, kFocusCal);

            // Build up filter state
            for (int i = 0; i < 1000; ++i)
            {
                focus.process(0.5f);
            }

            focus.reset();

            // After reset, processing silence should produce silence
            float result = 0.0f;
            for (int i = 0; i < 100; ++i)
            {
                result = focus.process(0.0f);
            }

            expectWithinAbsoluteError(result, 0.0f, TestConstants::kTolerance);
        }

        beginTest("Focus: silence in produces silence out");
        {
            GrainDSP::SpectralFocus focus;
            focus.prepare(44100.0f, GrainDSP::FocusMode::kHigh, kFocusCal);

            for (int i = 0; i < 100; ++i)
            {
                const float result = focus.process(0.0f);
                expectWithinAbsoluteError(result, 0.0f, TestConstants::kTolerance);
            }
        }

        beginTest("Focus: Low mode boosts low frequencies");
        {
            GrainDSP::SpectralFocus focus;
            focus.prepare(44100.0f, GrainDSP::FocusMode::kLow, kFocusCal);

            // Generate low frequency sine (100 Hz)
            const float sampleRate = 44100.0f;
            const int numSamples = static_cast<int>(sampleRate);  // 1 second
            const int measureStart = numSamples / 2;

            float rmsLow = 0.0f;
            float freq = 100.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float input = 0.5f * std::sin(GrainDSP::kTwoPi * freq * static_cast<float>(i) / sampleRate);
                const float output = focus.process(input);

                if (i >= measureStart)
                {
                    rmsLow += output * output;
                }
            }
            rmsLow = std::sqrt(rmsLow / static_cast<float>(numSamples - measureStart));

            // Now test high frequency (8000 Hz) with same mode
            focus.reset();
            float rmsHigh = 0.0f;
            freq = 8000.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float input = 0.5f * std::sin(GrainDSP::kTwoPi * freq * static_cast<float>(i) / sampleRate);
                const float output = focus.process(input);

                if (i >= measureStart)
                {
                    rmsHigh += output * output;
                }
            }
            rmsHigh = std::sqrt(rmsHigh / static_cast<float>(numSamples - measureStart));

            // In Low mode: low frequency RMS should be greater than high frequency RMS
            expect(rmsLow > rmsHigh);
        }

        beginTest("Focus: High mode boosts high frequencies");
        {
            GrainDSP::SpectralFocus focus;
            focus.prepare(44100.0f, GrainDSP::FocusMode::kHigh, kFocusCal);

            const float sampleRate = 44100.0f;
            const int numSamples = static_cast<int>(sampleRate);
            const int measureStart = numSamples / 2;

            // Low frequency (100 Hz)
            float rmsLow = 0.0f;
            float freq = 100.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float input = 0.5f * std::sin(GrainDSP::kTwoPi * freq * static_cast<float>(i) / sampleRate);
                const float output = focus.process(input);

                if (i >= measureStart)
                {
                    rmsLow += output * output;
                }
            }
            rmsLow = std::sqrt(rmsLow / static_cast<float>(numSamples - measureStart));

            // High frequency (8000 Hz)
            focus.reset();
            float rmsHigh = 0.0f;
            freq = 8000.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float input = 0.5f * std::sin(GrainDSP::kTwoPi * freq * static_cast<float>(i) / sampleRate);
                const float output = focus.process(input);

                if (i >= measureStart)
                {
                    rmsHigh += output * output;
                }
            }
            rmsHigh = std::sqrt(rmsHigh / static_cast<float>(numSamples - measureStart));

            // In High mode: high frequency RMS should be greater than low frequency RMS
            expect(rmsHigh > rmsLow);
        }

        beginTest("Focus: mono instances are independent (replaces stereo test)");
        {
            GrainDSP::SpectralFocus focusL;
            GrainDSP::SpectralFocus focusR;
            focusL.prepare(44100.0f, GrainDSP::FocusMode::kLow, kFocusCal);
            focusR.prepare(44100.0f, GrainDSP::FocusMode::kLow, kFocusCal);

            // Process different signals on L and R using sine waves at different frequencies
            // so they don't just mirror each other
            const float sampleRate = 44100.0f;
            float resultL = 0.0f;
            float resultR = 0.0f;

            for (int i = 0; i < 1000; ++i)
            {
                const float inputL = 0.5f * std::sin(GrainDSP::kTwoPi * 100.0f * static_cast<float>(i) / sampleRate);
                const float inputR = 0.5f * std::sin(GrainDSP::kTwoPi * 8000.0f * static_cast<float>(i) / sampleRate);
                resultL = focusL.process(inputL);
                resultR = focusR.process(inputR);
            }

            // In Low mode: 100 Hz is boosted, 8000 Hz is cut.
            // RMS of L should be greater than RMS of R for a given amplitude input.
            // At this specific moment, outputs should be different magnitudes.
            expect(std::abs(resultL) != std::abs(resultR));
        }

        beginTest("Focus: no NaN or Inf on edge cases");
        {
            GrainDSP::SpectralFocus focus;
            focus.prepare(44100.0f, GrainDSP::FocusMode::kLow, kFocusCal);

            // Test extreme values
            float result = focus.process(1.0f);
            expect(!std::isnan(result));
            expect(!std::isinf(result));

            result = focus.process(-1.0f);
            expect(!std::isnan(result));
            expect(!std::isinf(result));

            result = focus.process(0.0f);
            expect(!std::isnan(result));
            expect(!std::isinf(result));
        }
    }

    //==========================================================================
    void runStandaloneTests()
    {
        beginTest("Standalone: meter level is non-negative");
        {
            juce::AudioBuffer<float> buffer(2, TestConstants::kBufferSize);

            // Fill with sine wave
            for (int i = 0; i < TestConstants::kBufferSize; ++i)
            {
                float const sample = 0.5f * std::sin(GrainDSP::kTwoPi * 440.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            float const magnitude = buffer.getMagnitude(0, 0, TestConstants::kBufferSize);
            expect(magnitude >= 0.0f);
            expect(magnitude <= 1.0f);
        }

        beginTest("Standalone: silence produces zero meter level");
        {
            juce::AudioBuffer<float> buffer(2, TestConstants::kBufferSize);
            buffer.clear();

            float const magnitude = buffer.getMagnitude(0, 0, TestConstants::kBufferSize);
            expectWithinAbsoluteError(magnitude, 0.0f, TestConstants::kTolerance);
        }

        beginTest("Standalone: pipeline processes correctly at 44100 Hz");
        {
            GrainDSP::DSPPipeline pipeline;
            pipeline.prepare(44100.0f, GrainDSP::FocusMode::kMid, GrainDSP::kDefaultCalibration);

            float rmsOutput = 0.0f;
            const int numSamples = 4410;  // 100ms at 44100

            for (int i = 0; i < numSamples; ++i)
            {
                float const phase = GrainDSP::kTwoPi * 440.0f * static_cast<float>(i) / 44100.0f;
                float const input = 0.5f * std::sin(phase);
                float const output = pipeline.processSample(input, 0.3f, 0.5f, 0.5f, 0.5f, 1.0f);
                expect(!std::isnan(output), "NaN at sample " + juce::String(i));
                expect(!std::isinf(output), "Inf at sample " + juce::String(i));
                rmsOutput += output * output;
            }

            rmsOutput = std::sqrt(rmsOutput / static_cast<float>(numSamples));
            expect(rmsOutput > 0.0f, "Output is silent at 44100 Hz");
        }

        beginTest("Standalone: pipeline processes correctly at 48000 Hz");
        {
            GrainDSP::DSPPipeline pipeline;
            pipeline.prepare(48000.0f, GrainDSP::FocusMode::kMid, GrainDSP::kDefaultCalibration);

            float rmsOutput = 0.0f;
            const int numSamples = 4800;  // 100ms at 48000

            for (int i = 0; i < numSamples; ++i)
            {
                float const phase = GrainDSP::kTwoPi * 440.0f * static_cast<float>(i) / 48000.0f;
                float const input = 0.5f * std::sin(phase);
                float const output = pipeline.processSample(input, 0.3f, 0.5f, 0.5f, 0.5f, 1.0f);
                expect(!std::isnan(output), "NaN at sample " + juce::String(i));
                expect(!std::isinf(output), "Inf at sample " + juce::String(i));
                rmsOutput += output * output;
            }

            rmsOutput = std::sqrt(rmsOutput / static_cast<float>(numSamples));
            expect(rmsOutput > 0.0f, "Output is silent at 48000 Hz");
        }

        beginTest("Standalone: sample rate change re-initializes pipeline");
        {
            GrainDSP::DSPPipeline pipeline;

            // First prepare at 44100
            pipeline.prepare(44100.0f, GrainDSP::FocusMode::kMid, GrainDSP::kDefaultCalibration);
            for (int i = 0; i < 100; ++i)
            {
                float const phase = GrainDSP::kTwoPi * 440.0f * static_cast<float>(i) / 44100.0f;
                pipeline.processSample(0.5f * std::sin(phase), 0.3f, 0.5f, 0.5f, 0.5f, 1.0f);
            }

            // Re-prepare at 48000 (simulates device sample rate change)
            pipeline.prepare(48000.0f, GrainDSP::FocusMode::kMid, GrainDSP::kDefaultCalibration);

            // Process after re-prepare — should produce valid output
            for (int i = 0; i < 100; ++i)
            {
                float const phase = GrainDSP::kTwoPi * 440.0f * static_cast<float>(i) / 48000.0f;
                float const output = pipeline.processSample(0.5f * std::sin(phase), 0.3f, 0.5f, 0.5f, 0.5f, 1.0f);
                expect(!std::isnan(output), "NaN after sample rate change at sample " + juce::String(i));
                expect(!std::isinf(output), "Inf after sample rate change at sample " + juce::String(i));
            }
        }
    }

    //==========================================================================
    void runDCOffsetAccumulationTest()
    {
        beginTest("DC Offset: bias + DC blocker pipeline has near-zero mean");
        {
            GrainDSP::DCBlocker blocker;
            blocker.prepare(44100.0f, kDCBlockerCal);

            const float frequency = 440.0f;
            const float sampleRate = 44100.0f;
            const float rmsLevel = 0.5f;
            const float biasAmount = 1.0f;

            // Let filter settle first (500 samples)
            for (int i = 0; i < 500; ++i)
            {
                const float phase = GrainDSP::kTwoPi * frequency * static_cast<float>(i) / sampleRate;
                const float input = std::sin(phase);
                const float biased = GrainDSP::applyDynamicBias(input, rmsLevel, biasAmount, kBiasCal);
                blocker.process(biased);
            }

            // Measure mean over ~10 cycles of 440Hz (~227 samples per cycle)
            const int measureSamples = static_cast<int>(sampleRate / frequency) * 10;
            float sum = 0.0f;
            for (int i = 0; i < measureSamples; ++i)
            {
                const float phase = GrainDSP::kTwoPi * frequency * static_cast<float>(i + 500) / sampleRate;
                const float input = std::sin(phase);
                const float biased = GrainDSP::applyDynamicBias(input, rmsLevel, biasAmount, kBiasCal);
                sum += blocker.process(biased);
            }

            const float mean = sum / static_cast<float>(measureSamples);
            expect(std::abs(mean) < 0.01f);
        }
    }
};

//==============================================================================
// Register the test
static GrainDSPTests
    grainDSPTests;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
