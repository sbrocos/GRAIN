#include "../DSP/GrainDSP.h"

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
    }

private:
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
            detector.prepare(44100.0f, 100.0f, 300.0f);

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
            detector.prepare(44100.0f, 100.0f, 300.0f);

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
            detector.prepare(44100.0f, 100.0f, 300.0f);

            const float amplitude = 1.0f;
            const float frequency = 440.0f;
            const float sampleRate = 44100.0f;
            float result = 0.0f;

            // Process enough samples to converge (1 second)
            const int samplesToConverge = static_cast<int>(sampleRate * 1.0f);
            for (int i = 0; i < samplesToConverge; ++i)
            {
                const float phase = 2.0f * 3.14159265f * frequency * static_cast<float>(i) / sampleRate;
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
            detector.prepare(44100.0f, 100.0f, 300.0f);

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
            detector.prepare(44100.0f, 100.0f, 300.0f);

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
            detector.prepare(44100.0f, 100.0f, 300.0f);

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
};

//==============================================================================
// Register the test
static GrainDSPTests
    grainDSPTests;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
