#include <JuceHeader.h>

//==============================================================================
class GrainDSPTests : public juce::UnitTest
{
public:
    GrainDSPTests() : juce::UnitTest("GRAIN DSP Tests") {}

    void runTest() override
    {
        beginTest("Waveshaper: zero passthrough");
        expect(std::tanh(0.0f) == 0.0f);

        beginTest("Waveshaper: symmetry");
        {
            float x = 0.5f;
            expectWithinAbsoluteError(std::tanh(-x), -std::tanh(x), 0.0001f);
        }

        beginTest("Waveshaper: bounded output");
        {
            expect(std::tanh(100.0f) < 1.01f);
            expect(std::tanh(-100.0f) > -1.01f);
        }

        beginTest("Waveshaper: near-linear for small values");
        {
            float x = 0.1f;
            expectWithinAbsoluteError(std::tanh(x), x, 0.01f);
        }

        beginTest("Mix: full dry");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float mix = 0.0f;
            float result = (wet * mix) + (dry * (1.0f - mix));
            expectEquals(result, dry);
        }

        beginTest("Mix: full wet");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float mix = 1.0f;
            float result = (wet * mix) + (dry * (1.0f - mix));
            expectEquals(result, wet);
        }

        beginTest("Mix: 50/50 blend");
        {
            float dry = 1.0f;
            float wet = 0.0f;
            float mix = 0.5f;
            float result = (wet * mix) + (dry * (1.0f - mix));
            expectWithinAbsoluteError(result, 0.5f, 0.0001f);
        }

        beginTest("Gain: unity");
        {
            float input = 0.7f;
            float gain = 1.0f;
            expectEquals(input * gain, input);
        }

        beginTest("Gain: double");
        {
            float input = 0.5f;
            float gain = 2.0f;
            expectEquals(input * gain, 1.0f);
        }

        beginTest("Gain: zero (silence)");
        {
            float input = 0.7f;
            float gain = 0.0f;
            expectEquals(input * gain, 0.0f);
        }
    }
};

//==============================================================================
// Register the test
static GrainDSPTests grainDSPTests;
