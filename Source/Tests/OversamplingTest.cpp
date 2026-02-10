/*
  ==============================================================================

    OversamplingTest.cpp
    Unit tests for JUCE oversampling integration (Task 007).
    Validates upsample/downsample behavior, block sizes, and signal integrity.

  ==============================================================================
*/

#include "../DSP/Constants.h"

#include <juce_dsp/juce_dsp.h>

#include <JuceHeader.h>

//==============================================================================
namespace TestConstants
{
constexpr float kOsTolerance = 1e-5f;
}

//==============================================================================
class OversamplingTest : public juce::UnitTest
{
public:
    OversamplingTest() : juce::UnitTest("GRAIN Oversampling") {}

    void runTest() override
    {
        runSilenceTest();
        run2xBlockSizeTest();
        run4xBlockSizeTest();
        runLatencyTest();
        runSignalIntegrityTest();
    }

private:
    //==========================================================================
    void runSilenceTest()
    {
        beginTest("Oversampling: silence in produces silence out");

        juce::dsp::Oversampling<float> os(1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        constexpr int blockSize = 512;
        os.initProcessing(blockSize);

        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.clear();

        juce::dsp::AudioBlock<float> block(buffer);

        // Upsample
        auto upBlock = os.processSamplesUp(block);

        // Verify upsampled block is silent
        for (int i = 0; i < static_cast<int>(upBlock.getNumSamples()); ++i)
        {
            expectWithinAbsoluteError(upBlock.getSample(0, i), 0.0f, TestConstants::kOsTolerance);
        }

        // Downsample
        os.processSamplesDown(block);

        // Verify output is silent
        for (int i = 0; i < blockSize; ++i)
        {
            expectWithinAbsoluteError(buffer.getSample(0, i), 0.0f, TestConstants::kOsTolerance);
        }
    }

    //==========================================================================
    void run2xBlockSizeTest()
    {
        beginTest("Oversampling: 2x upsampled block has correct size");

        juce::dsp::Oversampling<float> os(1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        constexpr int blockSize = 256;
        os.initProcessing(blockSize);

        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.clear();

        juce::dsp::AudioBlock<float> block(buffer);
        auto upBlock = os.processSamplesUp(block);

        // 2× should double the samples
        expectEquals(static_cast<int>(upBlock.getNumSamples()), blockSize * 2);

        os.processSamplesDown(block);
    }

    //==========================================================================
    void run4xBlockSizeTest()
    {
        beginTest("Oversampling: 4x upsampled block has correct size");

        juce::dsp::Oversampling<float> os(1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        constexpr int blockSize = 256;
        os.initProcessing(blockSize);

        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.clear();

        juce::dsp::AudioBlock<float> block(buffer);
        auto upBlock = os.processSamplesUp(block);

        // 4× should quadruple the samples
        expectEquals(static_cast<int>(upBlock.getNumSamples()), blockSize * 4);

        os.processSamplesDown(block);
    }

    //==========================================================================
    void runLatencyTest()
    {
        beginTest("Oversampling: latency is within reasonable bounds");

        juce::dsp::Oversampling<float> os(1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        os.initProcessing(512);

        const float latency = os.getLatencyInSamples();

        // Polyphase IIR should have some latency, but not excessive
        expect(latency >= 0.0f);
        expect(latency < 50.0f);
    }

    //==========================================================================
    void runSignalIntegrityTest()
    {
        beginTest("Oversampling: signal passes through with minimal change");

        juce::dsp::Oversampling<float> os(1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        constexpr int blockSize = 512;
        os.initProcessing(blockSize);

        const float sampleRate = 44100.0f;
        const float freq = 440.0f;
        juce::AudioBuffer<float> buffer(1, blockSize);

        // Process multiple blocks to get past initial transient
        for (int rep = 0; rep < 10; ++rep)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const float phase = GrainDSP::kTwoPi * freq * static_cast<float>(i + rep * blockSize) / sampleRate;
                buffer.setSample(0, i, 0.5f * std::sin(phase));
            }

            juce::dsp::AudioBlock<float> block(buffer);
            auto upBlock = os.processSamplesUp(block);
            // No processing — just pass through
            os.processSamplesDown(block);
        }

        // After settling, signal should be close to original
        float rms = 0.0f;
        for (int i = 0; i < blockSize; ++i)
        {
            const float s = buffer.getSample(0, i);
            rms += s * s;
        }
        rms = std::sqrt(rms / static_cast<float>(blockSize));

        // Original RMS of 0.5 sine ≈ 0.354
        // After oversampling round-trip, should be close
        expect(rms > 0.2f);
        expect(rms < 0.5f);
    }
};

//==============================================================================
// Register the test
static OversamplingTest
    oversamplingTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
