/*
  ==============================================================================

    CalibrationTest.cpp
    Tests for CalibrationConfig (Task 007b).
    Verifies default values, extreme calibrations, and config differentiation.

  ==============================================================================
*/

#include "../DSP/CalibrationConfig.h"
#include "../DSP/GrainDSPPipeline.h"

#include <JuceHeader.h>

//==============================================================================
class CalibrationTest : public juce::UnitTest
{
public:
    CalibrationTest() : juce::UnitTest("GRAIN Calibration Config") {}

    void runTest() override
    {
        runDefaultCalibrationRegressionTest();
        runExtremeCalibrationTest();
        runDifferentCalibrationTest();
    }

private:
    //==========================================================================
    void runDefaultCalibrationRegressionTest()
    {
        beginTest("Default calibration matches original constants");
        {
            auto cal = GrainDSP::kDefaultCalibration;

            // RMS
            expectEquals(cal.rms.attackMs, 100.0f);
            expectEquals(cal.rms.releaseMs, 300.0f);

            // Bias
            expectEquals(cal.bias.amount, 0.3f);
            expectEquals(cal.bias.scale, 0.1f);

            // Waveshaper
            expectEquals(cal.waveshaper.driveMin, 0.1f);
            expectEquals(cal.waveshaper.driveMax, 0.4f);

            // Warmth
            expectEquals(cal.warmth.depth, 0.22f);

            // Focus
            expectEquals(cal.focus.lowShelfFreq, 200.0f);
            expectEquals(cal.focus.highShelfFreq, 4000.0f);
            expectEquals(cal.focus.shelfGainDb, 2.8f);
            expectEquals(cal.focus.shelfQ, 0.707f);

            // DC Blocker
            expectEquals(cal.dcBlocker.cutoffHz, 5.0f);
        }
    }

    //==========================================================================
    void runExtremeCalibrationTest()
    {
        beginTest("Extreme calibration produces no NaN/Inf");
        {
            GrainDSP::CalibrationConfig extreme;
            extreme.rms.attackMs = 1.0f;      // Very fast
            extreme.rms.releaseMs = 5000.0f;  // Very slow
            extreme.bias.amount = 1.0f;       // Max bias
            extreme.bias.scale = 1.0f;        // Max scale
            extreme.waveshaper.driveMin = 0.01f;
            extreme.waveshaper.driveMax = 2.0f;  // Beyond safe zone
            extreme.warmth.depth = 1.0f;         // Max warmth
            extreme.focus.shelfGainDb = 12.0f;   // Aggressive shelving

            GrainDSP::DSPPipeline pipeline;
            pipeline.prepare(44100.0f, GrainDSP::FocusMode::kMid, extreme);

            // Process 1000 samples of sine wave
            for (int i = 0; i < 1000; ++i)
            {
                float sample = std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(i) / 44100.0f);
                float result = pipeline.processSample(sample, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f);
                expect(!std::isnan(result), "NaN at sample " + juce::String(i));
                expect(!std::isinf(result), "Inf at sample " + juce::String(i));
            }
        }
    }

    //==========================================================================
    void runDifferentCalibrationTest()
    {
        beginTest("Different calibrations produce different output");
        {
            GrainDSP::DSPPipeline pipelineA;
            GrainDSP::DSPPipeline pipelineB;

            GrainDSP::CalibrationConfig configA = GrainDSP::kDefaultCalibration;
            GrainDSP::CalibrationConfig configB = GrainDSP::kDefaultCalibration;
            configB.warmth.depth = 0.8f;  // Much more warmth

            pipelineA.prepare(44100.0f, GrainDSP::FocusMode::kMid, configA);
            pipelineB.prepare(44100.0f, GrainDSP::FocusMode::kMid, configB);

            float sample = 0.5f;
            float resultA = pipelineA.processSample(sample, 0.5f, 0.3f, 0.5f, 1.0f, 1.0f);
            float resultB = pipelineB.processSample(sample, 0.5f, 0.3f, 0.5f, 1.0f, 1.0f);

            expect(std::abs(resultA - resultB) > 1e-6f,
                   "Expected different outputs, got A=" + juce::String(resultA) + " B=" + juce::String(resultB));
        }
    }
};

//==============================================================================
// Register the test
static CalibrationTest
    calibrationTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
