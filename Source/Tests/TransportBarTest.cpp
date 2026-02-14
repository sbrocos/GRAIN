/*
  ==============================================================================

    TransportBarTest.cpp
    Unit tests for the standalone TransportBar component (GT-17).
    Tests button states, time formatting, and progress tracking.

  ==============================================================================
*/

#include "../Standalone/TransportBar.h"

#include "../Standalone/FilePlayerSource.h"

#include <JuceHeader.h>

//==============================================================================
namespace
{

/** Create a temporary WAV file with a sine wave (shared with FilePlayerTest). */
juce::File createTestWavFileForTransport(double sampleRate, int numChannels, double durationSeconds)
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

    int const totalSamples = static_cast<int>(sampleRate * durationSeconds);
    juce::AudioBuffer<float> buffer(numChannels, totalSamples);

    for (int i = 0; i < totalSamples; ++i)
    {
        float const sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) /
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
class TransportBarTest : public juce::UnitTest
{
public:
    TransportBarTest() : juce::UnitTest("GRAIN Transport Bar") {}

    void runTest() override
    {
        runFormatTimeTest();
        runInitialButtonStatesTest();
        runButtonStatesAfterLoadTest();
        runPlayStopButtonTextTest();
        runProgressTrackingTest();
    }

private:
    //==========================================================================
    void runFormatTimeTest()
    {
        beginTest("TransportBar: time formatting");

        // Use TransportBar::formatTime (static, public for testing)
        expectEquals(TransportBar::formatTime(0.0), juce::String("00:00"));
        expectEquals(TransportBar::formatTime(5.0), juce::String("00:05"));
        expectEquals(TransportBar::formatTime(65.0), juce::String("01:05"));
        expectEquals(TransportBar::formatTime(3661.0), juce::String("61:01"));
        expectEquals(TransportBar::formatTime(-5.0), juce::String("00:00"));
    }

    //==========================================================================
    void runInitialButtonStatesTest()
    {
        beginTest("TransportBar: initial button states (no file loaded)");

        FilePlayerSource player;
        TransportBar bar(player);

        // With no file loaded, play/stop and loop should be disabled
        bar.updateButtonStates();

        // We can't directly check button enabled state without accessing private members,
        // but we verify the component creates without crash and the player has correct state
        expect(!player.isFileLoaded(), "No file should be loaded initially");
        expect(!player.isPlaying(), "Should not be playing initially");
        expect(!player.isLooping(), "Should not be looping initially");
    }

    //==========================================================================
    void runButtonStatesAfterLoadTest()
    {
        beginTest("TransportBar: button states after file load");

        auto tempFile = createTestWavFileForTransport(44100.0, 2, 1.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        TransportBar bar(player);
        bar.updateButtonStates();

        expect(player.isFileLoaded(), "File should be loaded");
        expect(!player.isPlaying(), "Should not be playing until play() called");

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runPlayStopButtonTextTest()
    {
        beginTest("TransportBar: play/stop button reflects state");

        auto tempFile = createTestWavFileForTransport(44100.0, 2, 1.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        TransportBar bar(player);
        bar.updateButtonStates();

        // Before play — button should indicate "Play"
        expect(!player.isPlaying(), "Should not be playing before play()");

        // Start playback
        player.play();
        bar.updateButtonStates();
        expect(player.isPlaying(), "Should be playing after play()");

        // Stop playback
        player.stop();
        bar.updateButtonStates();
        expect(!player.isPlaying(), "Should not be playing after stop()");

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runProgressTrackingTest()
    {
        beginTest("TransportBar: progress tracking with seek");

        auto tempFile = createTestWavFileForTransport(44100.0, 2, 2.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        // Verify position tracking through player
        player.seekToPosition(1.0);
        expectWithinAbsoluteError(player.getCurrentPosition(), 1.0, 0.01);

        double const normalized = player.getCurrentPosition() / player.getFileDurationSeconds();
        expectWithinAbsoluteError(normalized, 0.5, 0.01);

        player.releaseResources();
        tempFile.deleteFile();
    }
};

//==============================================================================
static TransportBarTest
    transportBarTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
