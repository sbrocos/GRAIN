#include <JuceHeader.h>
#include "../DSP/GrainDSP.h"

//==============================================================================
namespace TestConstants
{
    constexpr float TOLERANCE = 1e-5f;
    constexpr float CLICK_THRESHOLD = 0.01f;
    constexpr int BUFFER_SIZE = 512;
}

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
    }

private:
    //==========================================================================
    void runWaveshaperTests()
    {
        beginTest("Waveshaper: zero passthrough");
        expectWithinAbsoluteError(GrainDSP::applyWaveshaper(0.0f, 0.0f), 0.0f, TestConstants::TOLERANCE);

        beginTest("Waveshaper: symmetry");
        {
            float x = 0.5f;
            float drive = 0.5f;
            expectWithinAbsoluteError(
                GrainDSP::applyWaveshaper(-x, drive),
                -GrainDSP::applyWaveshaper(x, drive),
                TestConstants::TOLERANCE
            );
        }

        beginTest("Waveshaper: bounded output");
        {
            expect(GrainDSP::applyWaveshaper(100.0f, 1.0f) < 1.01f);
            expect(GrainDSP::applyWaveshaper(-100.0f, 1.0f) > -1.01f);
        }

        beginTest("Waveshaper: near-linear for small values");
        {
            float x = 0.05f;
            float result = GrainDSP::applyWaveshaper(x, 0.0f);  // drive = 0, gain = 1x
            expectWithinAbsoluteError(result, x, 0.01f);
        }
    }

    //==========================================================================
    void runMixTests()
    {
        beginTest("Mix: full dry (mix = 0)");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float result = GrainDSP::applyMix(dry, wet, 0.0f);
            expectWithinAbsoluteError(result, dry, TestConstants::TOLERANCE);
        }

        beginTest("Mix: full wet (mix = 1)");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float result = GrainDSP::applyMix(dry, wet, 1.0f);
            expectWithinAbsoluteError(result, wet, TestConstants::TOLERANCE);
        }

        beginTest("Mix: 50/50 blend");
        {
            float dry = 1.0f;
            float wet = 0.0f;
            float result = GrainDSP::applyMix(dry, wet, 0.5f);
            expectWithinAbsoluteError(result, 0.5f, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runGainTests()
    {
        beginTest("Gain: unity");
        {
            float input = 0.7f;
            float result = GrainDSP::applyGain(input, 1.0f);
            expectWithinAbsoluteError(result, input, TestConstants::TOLERANCE);
        }

        beginTest("Gain: double (+6dB)");
        {
            float input = 0.5f;
            float result = GrainDSP::applyGain(input, 2.0f);
            expectWithinAbsoluteError(result, 1.0f, TestConstants::TOLERANCE);
        }

        beginTest("Gain: zero (silence)");
        {
            float input = 0.7f;
            float result = GrainDSP::applyGain(input, 0.0f);
            expectWithinAbsoluteError(result, 0.0f, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runBypassTests()
    {
        // Note: Bypass is implemented via mix smoothing (bypass ON → mix = 0)
        // These tests verify that mix = 0 produces dry signal (bypass behavior)

        beginTest("Bypass behavior: mix = 0 returns dry signal");
        {
            float dry = 0.7f;
            float wet = 0.3f;  // Different from dry
            float mix = 0.0f; // Simulates bypass ON
            float result = GrainDSP::applyMix(dry, wet, mix);
            expectWithinAbsoluteError(result, dry, TestConstants::TOLERANCE);
        }

        beginTest("Bypass behavior: full processing when mix > 0");
        {
            float dry = 0.7f;
            float wet = 0.3f;
            float mix = 0.5f; // Normal operation
            float result = GrainDSP::applyMix(dry, wet, mix);
            float expected = (wet * mix) + (dry * (1.0f - mix));
            expectWithinAbsoluteError(result, expected, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runBufferTests()
    {
        beginTest("Buffer processing: stability with constant input");
        {
            std::vector<float> buffer(TestConstants::BUFFER_SIZE, 0.5f);
            float drive = 0.5f;
            float mix = 0.5f;
            float gain = 1.0f;

            // Process buffer
            for (int i = 0; i < TestConstants::BUFFER_SIZE; ++i)
            {
                float dry = buffer[i];
                float wet = GrainDSP::applyWaveshaper(dry, drive);
                float mixed = GrainDSP::applyMix(dry, wet, mix);
                buffer[i] = GrainDSP::applyGain(mixed, gain);
            }

            // All samples should be identical (constant input -> constant output)
            float expected = buffer[0];
            for (int i = 1; i < TestConstants::BUFFER_SIZE; ++i)
            {
                expectWithinAbsoluteError(buffer[i], expected, TestConstants::TOLERANCE);
            }
        }

        beginTest("Buffer processing: no state leak between samples");
        {
            float drive = 0.5f;

            // Process same value multiple times
            float result1 = GrainDSP::applyWaveshaper(0.5f, drive);
            float result2 = GrainDSP::applyWaveshaper(0.5f, drive);
            float result3 = GrainDSP::applyWaveshaper(0.5f, drive);

            expectWithinAbsoluteError(result1, result2, TestConstants::TOLERANCE);
            expectWithinAbsoluteError(result2, result3, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runDiscontinuityTests()
    {
        beginTest("Parameter change: no discontinuities on silent input");
        {
            // Silent input - any click would be clearly audible
            std::vector<float> buffer(TestConstants::BUFFER_SIZE, 0.0f);

            float prevSample = 0.0f;
            bool hasClick = false;

            // Simulate abrupt parameter change: drive 0 -> 1
            for (int i = 0; i < TestConstants::BUFFER_SIZE; ++i)
            {
                // Abrupt change at midpoint
                float drive = (i < TestConstants::BUFFER_SIZE / 2) ? 0.0f : 1.0f;

                float wet = GrainDSP::applyWaveshaper(buffer[i], drive);
                float output = GrainDSP::applyMix(buffer[i], wet, 0.5f);

                float delta = std::abs(output - prevSample);
                if (delta > TestConstants::CLICK_THRESHOLD)
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
};

//==============================================================================
// Register the test
static GrainDSPTests grainDSPTests;
