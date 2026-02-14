/*
  ==============================================================================

    FilePlayerTest.cpp
    Unit tests for the standalone FilePlayerSource (GT-15, GT-16).
    Tests file loading, validation, metadata, thumbnail, sample rate handling,
    and transport controls (play/stop/loop/seek).

  ==============================================================================
*/

#include "../Standalone/FilePlayerSource.h"

#include <JuceHeader.h>

//==============================================================================
namespace
{

/** Create a temporary WAV file with a sine wave. */
juce::File createTestWavFile(double sampleRate, int numChannels, double durationSeconds, float frequency = 440.0f)
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
        float const sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * static_cast<float>(i) /
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

/** Create a temporary AIFF file with a sine wave. */
juce::File createTestAiffFile(double sampleRate, int numChannels, double durationSeconds, float frequency = 440.0f)
{
    auto tempFile = juce::File::createTempFile(".aiff");
    std::unique_ptr<juce::OutputStream> outputStream = tempFile.createOutputStream();

    if (outputStream == nullptr)
    {
        return {};
    }

    juce::AiffAudioFormat aiffFormat;
    auto options =
        juce::AudioFormatWriterOptions().withSampleRate(sampleRate).withNumChannels(numChannels).withBitsPerSample(16);
    auto writer = aiffFormat.createWriterFor(outputStream, options);

    if (writer == nullptr)
    {
        return {};
    }

    int const totalSamples = static_cast<int>(sampleRate * durationSeconds);
    juce::AudioBuffer<float> buffer(numChannels, totalSamples);

    for (int i = 0; i < totalSamples; ++i)
    {
        float const sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * static_cast<float>(i) /
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
class FilePlayerTest : public juce::UnitTest
{
public:
    FilePlayerTest() : juce::UnitTest("GRAIN File Player") {}

    void runTest() override
    {
        // GT-15: File loading tests
        runLoadValidWavTest();
        runLoadValidAiffTest();
        runRejectInvalidFileTest();
        runRejectNonExistentFileTest();
        runThumbnailGenerationTest();
        runSampleRateMismatchTest();
        runUnloadFileTest();
        runMetadataAccuracyTest();

        // GT-16: Transport tests
        runPlayStartsFromPositionZeroTest();
        runStopPausesAtCurrentPositionTest();
        runLoopRestartsTest();
        runSeekSetsPositionTest();
        runSeekProducesNoNanInfTest();
        runFileAudioReplacesInputTest();
    }

private:
    //==========================================================================
    void runLoadValidWavTest()
    {
        beginTest("FilePlayer: load valid WAV file");

        auto tempFile = createTestWavFile(44100.0, 2, 1.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        bool const loaded = player.loadFile(tempFile);

        expect(loaded, "loadFile returned false for valid WAV");
        expect(player.isFileLoaded(), "isFileLoaded returned false");
        expect(player.getLastError().isEmpty(), "Unexpected error: " + player.getLastError());
        expectEquals(player.getFileSampleRate(), 44100.0);
        expectEquals(player.getFileNumChannels(), 2);
        expectEquals(player.getFileLengthInSamples(), static_cast<juce::int64>(44100));

        tempFile.deleteFile();
    }

    //==========================================================================
    void runLoadValidAiffTest()
    {
        beginTest("FilePlayer: load valid AIFF file");

        auto tempFile = createTestAiffFile(48000.0, 1, 0.5);
        expect(tempFile.existsAsFile(), "Failed to create test AIFF file");

        FilePlayerSource player;
        bool const loaded = player.loadFile(tempFile);

        expect(loaded, "loadFile returned false for valid AIFF");
        expect(player.isFileLoaded(), "isFileLoaded returned false");
        expectEquals(player.getFileSampleRate(), 48000.0);
        expectEquals(player.getFileNumChannels(), 1);
        expectEquals(player.getFileLengthInSamples(), static_cast<juce::int64>(24000));

        tempFile.deleteFile();
    }

    //==========================================================================
    void runRejectInvalidFileTest()
    {
        beginTest("FilePlayer: reject invalid file");

        auto tempFile = juce::File::createTempFile(".wav");
        tempFile.replaceWithText("this is not audio data");

        FilePlayerSource player;
        bool const loaded = player.loadFile(tempFile);

        expect(!loaded, "loadFile should return false for invalid file");
        expect(!player.isFileLoaded(), "isFileLoaded should be false");
        expect(player.getLastError().isNotEmpty(), "Expected an error message");

        tempFile.deleteFile();
    }

    //==========================================================================
    void runRejectNonExistentFileTest()
    {
        beginTest("FilePlayer: reject non-existent file");

        FilePlayerSource player;
        bool const loaded = player.loadFile(juce::File("/nonexistent/path/audio.wav"));

        expect(!loaded, "loadFile should return false for non-existent file");
        expect(!player.isFileLoaded(), "isFileLoaded should be false");
        expect(player.getLastError().isNotEmpty(), "Expected an error message");
    }

    //==========================================================================
    void runThumbnailGenerationTest()
    {
        beginTest("FilePlayer: thumbnail generation completes");

        auto tempFile = createTestWavFile(44100.0, 2, 0.5);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);

        // Poll for thumbnail completion (async generation)
        bool ready = false;

        for (int attempt = 0; attempt < 100; ++attempt)
        {
            if (player.isThumbnailReady())
            {
                ready = true;
                break;
            }

            juce::Thread::sleep(10);
        }

        expect(ready, "Thumbnail did not finish generating within 1 second");
        expect(player.getThumbnail().getTotalLength() > 0.0, "Thumbnail has zero length");

        tempFile.deleteFile();
    }

    //==========================================================================
    void runSampleRateMismatchTest()
    {
        beginTest("FilePlayer: sample rate mismatch handling");

        auto tempFile = createTestWavFile(96000.0, 2, 0.5);
        expect(tempFile.existsAsFile(), "Failed to create 96kHz test WAV file");

        FilePlayerSource player;
        bool const loaded = player.loadFile(tempFile);

        expect(loaded, "loadFile should succeed for 96kHz file");
        expectEquals(player.getFileSampleRate(), 96000.0);

        // prepareToPlay with a different device sample rate should not crash
        player.prepareToPlay(44100.0, 512);
        expect(player.getTransportSource() != nullptr, "Transport source should be available");

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runUnloadFileTest()
    {
        beginTest("FilePlayer: unload resets state");

        auto tempFile = createTestWavFile(44100.0, 2, 1.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        expect(player.isFileLoaded(), "File should be loaded");

        player.unloadFile();

        expect(!player.isFileLoaded(), "isFileLoaded should be false after unload");
        expectEquals(player.getFileSampleRate(), 0.0);
        expectEquals(player.getFileLengthInSamples(), static_cast<juce::int64>(0));
        expectEquals(player.getFileNumChannels(), 0);
        expect(player.getTransportSource() == nullptr, "Transport source should be nullptr after unload");

        tempFile.deleteFile();
    }

    //==========================================================================
    void runMetadataAccuracyTest()
    {
        beginTest("FilePlayer: metadata accuracy");

        auto tempFile = createTestWavFile(44100.0, 2, 2.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);

        expectEquals(player.getFileLengthInSamples(), static_cast<juce::int64>(88200));
        expectWithinAbsoluteError(player.getFileDurationSeconds(), 2.0, 0.001);
        expect(player.getLoadedFile() == tempFile, "Loaded file path mismatch");

        tempFile.deleteFile();
    }

    //==========================================================================
    // GT-16: Transport tests
    //==========================================================================
    void runPlayStartsFromPositionZeroTest()
    {
        beginTest("Transport: play starts from position 0");

        auto tempFile = createTestWavFile(44100.0, 2, 1.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        expectWithinAbsoluteError(player.getCurrentPosition(), 0.0, 0.001);

        player.play();
        expect(player.isPlaying(), "Player should be playing after play()");

        // Position should still be near zero right after starting
        expectWithinAbsoluteError(player.getCurrentPosition(), 0.0, 0.05);

        player.stop();
        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runStopPausesAtCurrentPositionTest()
    {
        beginTest("Transport: stop pauses at current position");

        auto tempFile = createTestWavFile(44100.0, 2, 2.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        // Seek to 0.5s, then play and immediately stop
        player.seekToPosition(0.5);
        player.play();

        // Process a small amount to advance position
        juce::AudioBuffer<float> buffer(2, 512);
        juce::AudioSourceChannelInfo channelInfo(&buffer, 0, 512);
        player.getNextAudioBlock(channelInfo);

        player.stop();
        expect(!player.isPlaying(), "Player should not be playing after stop()");

        // Position should be approximately 0.5s + the processed samples
        double const pos = player.getCurrentPosition();
        expect(pos >= 0.5, "Position should be at least 0.5s after seeking there");

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runLoopRestartsTest()
    {
        beginTest("Transport: loop mode setting");

        auto tempFile = createTestWavFile(44100.0, 2, 0.5);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        // Default should be no looping
        expect(!player.isLooping(), "Default should be non-looping");

        // Enable looping
        player.setLooping(true);
        expect(player.isLooping(), "isLooping should return true after setLooping(true)");

        // Disable looping
        player.setLooping(false);
        expect(!player.isLooping(), "isLooping should return false after setLooping(false)");

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runSeekSetsPositionTest()
    {
        beginTest("Transport: seek sets position correctly");

        auto tempFile = createTestWavFile(44100.0, 2, 2.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        // Seek to 1.0 second
        player.seekToPosition(1.0);
        expectWithinAbsoluteError(player.getCurrentPosition(), 1.0, 0.01);

        // Seek to beginning
        player.seekToPosition(0.0);
        expectWithinAbsoluteError(player.getCurrentPosition(), 0.0, 0.01);

        // Seek beyond end (should clamp)
        player.seekToPosition(10.0);
        expectWithinAbsoluteError(player.getCurrentPosition(), 2.0, 0.01);

        // Seek to negative (should clamp to 0)
        player.seekToPosition(-1.0);
        expectWithinAbsoluteError(player.getCurrentPosition(), 0.0, 0.01);

        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runSeekProducesNoNanInfTest()
    {
        beginTest("Transport: seek produces no NaN/Inf in output");

        auto tempFile = createTestWavFile(44100.0, 2, 1.0);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        player.play();

        // Process a block, then seek, then process another block
        juce::AudioBuffer<float> buffer(2, 512);

        juce::AudioSourceChannelInfo channelInfo(&buffer, 0, 512);
        player.getNextAudioBlock(channelInfo);

        // Seek mid-playback
        player.seekToPosition(0.5);

        // Process after seek — must not produce NaN or Inf
        buffer.clear();
        player.getNextAudioBlock(channelInfo);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto const* samples = buffer.getReadPointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                expect(!std::isnan(samples[i]), "NaN detected after seek at sample " + juce::String(i));
                expect(!std::isinf(samples[i]), "Inf detected after seek at sample " + juce::String(i));
            }
        }

        player.stop();
        player.releaseResources();
        tempFile.deleteFile();
    }

    //==========================================================================
    void runFileAudioReplacesInputTest()
    {
        beginTest("Transport: file audio replaces device input (not silence)");

        auto tempFile = createTestWavFile(44100.0, 2, 1.0, 440.0f);
        expect(tempFile.existsAsFile(), "Failed to create test WAV file");

        FilePlayerSource player;
        player.loadFile(tempFile);
        player.prepareToPlay(44100.0, 512);

        player.play();

        // Get audio from the player
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::AudioSourceChannelInfo channelInfo(&buffer, 0, 512);
        player.getNextAudioBlock(channelInfo);

        // Output should NOT be silence (we loaded a 440Hz sine)
        float rms = buffer.getRMSLevel(0, 0, 512);
        expect(rms > 0.01f, "File audio should not be silent, RMS = " + juce::String(rms));

        player.stop();
        player.releaseResources();
        tempFile.deleteFile();
    }
};

//==============================================================================
static FilePlayerTest
    filePlayerTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
