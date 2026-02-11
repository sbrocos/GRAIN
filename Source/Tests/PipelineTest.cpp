/*
  ==============================================================================

    PipelineTest.cpp
    Integration tests for the GrainDSP::DSPPipeline (per-channel processing).
    Verifies the full signal chain works correctly end-to-end.

  ==============================================================================
*/

#include "../DSP/CalibrationConfig.h"
#include "../DSP/GrainDSPPipeline.h"

#include <JuceHeader.h>
#include <array>

//==============================================================================
class PipelineTest : public juce::UnitTest
{
public:
    PipelineTest() : juce::UnitTest("GRAIN Pipeline Integration") {}

    void runTest() override
    {
        runSilenceTest();
        runMixZeroTest();
        runExtremeInputTest();
        runLevelMatchTest();
    }

private:
    //==========================================================================
    void runSilenceTest()
    {
        beginTest("Pipeline: silence in produces silence out");

        GrainDSP::DSPPipeline pipeline;
        pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid, GrainDSP::kDefaultCalibration);

        for (int i = 0; i < 512; ++i)
        {
            float result = pipeline.processSample(0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 1.0f);
            expectWithinAbsoluteError(result, 0.0f, 1e-5f);
        }
    }

    //==========================================================================
    void runMixZeroTest()
    {
        beginTest("Pipeline: mix=0 returns dry signal (sine)");

        GrainDSP::DSPPipeline pipeline;
        pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid, GrainDSP::kDefaultCalibration);

        // Use a sine wave instead of DC to avoid DC blocker convergence to 0
        float rmsInput = 0.0f;
        float rmsOutput = 0.0f;
        int numSamples = 4410;  // 100ms

        for (int i = 0; i < numSamples; ++i)
        {
            float phase = GrainDSP::kTwoPi * 440.0f * static_cast<float>(i) / 44100.0f;
            float input = 0.5f * std::sin(phase);
            float result = pipeline.processSample(input, 0.3f, 0.5f, 0.5f, 0.0f, 1.0f);
            rmsInput += input * input;
            rmsOutput += result * result;
        }

        rmsInput = std::sqrt(rmsInput / static_cast<float>(numSamples));
        rmsOutput = std::sqrt(rmsOutput / static_cast<float>(numSamples));

        // mix=0: output should closely match input (DC blocker has minimal effect on AC)
        expectWithinAbsoluteError(rmsOutput, rmsInput, 0.01f);
    }

    //==========================================================================
    void runExtremeInputTest()
    {
        beginTest("Pipeline: no NaN/Inf on extreme input");

        GrainDSP::DSPPipeline pipeline;
        pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid, GrainDSP::kDefaultCalibration);

        std::array<float, 6> extremes = {0.0f, 1.0f, -1.0f, 100.0f, -100.0f, 1e-30f};
        for (float input : extremes)
        {
            float result = pipeline.processSample(input, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f);
            expect(!std::isnan(result), "Output is NaN for input " + juce::String(input));
            expect(!std::isinf(result), "Output is Inf for input " + juce::String(input));
        }
    }

    //==========================================================================
    void runLevelMatchTest()
    {
        beginTest("Pipeline: output level approximately matches input at low settings");

        GrainDSP::DSPPipeline pipeline;
        pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid, GrainDSP::kDefaultCalibration);

        // Low drive, full mix, unity gain
        float rmsSum = 0.0f;
        float inputRmsSum = 0.0f;
        int numSamples = 44100;

        for (int i = 0; i < numSamples; ++i)
        {
            float phase = GrainDSP::kTwoPi * 440.0f * static_cast<float>(i) / 44100.0f;
            float input = 0.5f * std::sin(phase);
            float output = pipeline.processSample(input, 0.1f, 0.1f, 0.0f, 1.0f, 1.0f);
            rmsSum += output * output;
            inputRmsSum += input * input;
        }

        float outputRms = std::sqrt(rmsSum / static_cast<float>(numSamples));
        float inputRms = std::sqrt(inputRmsSum / static_cast<float>(numSamples));

        // At low settings, output should be within ~3dB of input
        expect(outputRms > inputRms * 0.5f,
               "Output too quiet: " + juce::String(outputRms) + " < " + juce::String(inputRms * 0.5f));
        expect(outputRms < inputRms * 2.0f,
               "Output too loud: " + juce::String(outputRms) + " > " + juce::String(inputRms * 2.0f));
    }
};

static PipelineTest pipelineTest;
