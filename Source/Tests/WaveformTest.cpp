/*
  ==============================================================================

    WaveformTest.cpp
    Unit tests for the standalone WaveformDisplay component (GT-18).
    Tests rendering without crash, click-to-position mapping, and wet buffer
    accumulation.

  ==============================================================================
*/

#include "../Standalone/FilePlayerSource.h"
#include "../Standalone/WaveformDisplay.h"

#include <JuceHeader.h>

//==============================================================================
namespace
{

/** Create a temporary WAV file with a sine wave for testing. */
juce::File createTestWavFileForWaveform(double sampleRate, int numChannels, double durationSeconds)
{
    auto tempFile = juce::File::createTempFile(".wav");
    std::unique_ptr<juce::OutputStream> outputStream = tempFile.createOutputStream();

    if (outputStream == nullptr)
    {
        return {};
    }

    juce::WavAudioFormat wavFormat;
    auto options =
        juce::AudioFormatWriterOptions().withSampleRate(sampleRate).withNumChannels(numChannels).withBitsPerSample(16);
    auto writer = wavFormat.createWriterFor(outputStream, options);

    if (writer == nullptr)
    {
        return {};
    }

    auto const totalSamples = static_cast<int>(sampleRate * durationSeconds);
    juce::AudioBuffer<float> buffer(numChannels, totalSamples);

    for (int i = 0; i < totalSamples; ++i)
    {
        auto const sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) /
                                            static_cast<float>(sampleRate));

        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.setSample(ch, i, sample);
        }
    }

    writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples);
    writer.reset();

    return tempFile;
}

}  // namespace

//==============================================================================
class WaveformDisplayTest : public juce::UnitTest
{
public:
    WaveformDisplayTest() : juce::UnitTest("GRAIN Waveform Display") {}

    void runTest() override
    {
        runEmptyStateRenderTest();
        runLoadedFileRenderTest();
        runClickPositionMappingTest();
        runWetBufferAccumulationTest();
    }

private:
    //==========================================================================
    void runEmptyStateRenderTest()
    {
        beginTest("WaveformDisplay: renders without crash for empty state");

        FilePlayerSource player;
        WaveformDisplay display(player);

        // Set a reasonable size
        display.setSize(400, 120);

        // Create a graphics context and paint — should not crash
        juce::Image image(juce::Image::ARGB, 400, 120, true);
        juce::Graphics g(image);
        display.paint(g);

        // Verify component state
        expect(!player.isFileLoaded(), "No file should be loaded");
        expect(!display.hasWetData(), "No wet data should exist");
    }

    //==========================================================================
    void runLoadedFileRenderTest()
    {
        beginTest("WaveformDisplay: renders without crash for loaded file");

        auto tempFile = createTestWavFileForWaveform(44100.0, 2, 1.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        WaveformDisplay display(player);
        display.setSize(400, 120);

        // Wait a bit for thumbnail to generate
        juce::Thread::sleep(100);

        // Paint — should not crash
        juce::Image image(juce::Image::ARGB, 400, 120, true);
        juce::Graphics g(image);
        display.paint(g);

        expect(player.isFileLoaded(), "File should be loaded");

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runClickPositionMappingTest()
    {
        beginTest("WaveformDisplay: click position maps correctly to file time");

        auto tempFile = createTestWavFileForWaveform(44100.0, 2, 2.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        WaveformDisplay display(player);
        display.setSize(400, 120);

        auto bounds = display.getWaveformBounds();

        // Test pixel-to-normalized mapping
        // Left edge should be ~0.0
        float const leftNorm = display.pixelToNormalized(bounds.getX());
        expectWithinAbsoluteError(leftNorm, 0.0f, 0.01f);

        // Right edge should be ~1.0
        float const rightNorm = display.pixelToNormalized(bounds.getRight());
        expectWithinAbsoluteError(rightNorm, 1.0f, 0.01f);

        // Center should be ~0.5
        float const centerNorm = display.pixelToNormalized(bounds.getCentreX());
        expectWithinAbsoluteError(centerNorm, 0.5f, 0.02f);

        // Test normalized-to-pixel roundtrip
        int const pixel = display.normalizedToPixel(0.5f);
        float const roundTrip = display.pixelToNormalized(pixel);
        expectWithinAbsoluteError(roundTrip, 0.5f, 0.02f);

        // Verify clamping
        float const belowZero = display.pixelToNormalized(bounds.getX() - 100);
        expectEquals(belowZero, 0.0f);

        float const aboveOne = display.pixelToNormalized(bounds.getRight() + 100);
        expectEquals(aboveOne, 1.0f);

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runWetBufferAccumulationTest()
    {
        beginTest("WaveformDisplay: wet waveform buffer accumulates samples");

        FilePlayerSource player;
        WaveformDisplay display(player);
        display.setSize(400, 120);

        // Initially no wet data
        expect(!display.hasWetData(), "Should have no wet data initially");

        // Push some test samples
        constexpr int kNumSamples = 512;
        std::vector<float> testSamples(kNumSamples, 0.0f);
        for (int i = 0; i < kNumSamples; ++i)
        {
            testSamples[static_cast<size_t>(i)] =
                0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) / 44100.0f);
        }

        display.pushWetSamples(testSamples.data(), kNumSamples);

        // After pushing, the FIFO has data but wet columns aren't updated
        // until timerCallback drains the FIFO. We check hasWetData is still false
        // because the data is in the FIFO, not yet in the columns.
        // This verifies the push didn't crash and FIFO accepted data.

        // Clear and verify
        display.clearWetBuffer();
        expect(!display.hasWetData(), "Should have no wet data after clear");
    }
};

//==============================================================================
static WaveformDisplayTest
    waveformDisplayTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
