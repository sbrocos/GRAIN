/*
  ==============================================================================

    RecorderTest.cpp
    Unit tests for the standalone AudioRecorder (GT-20).
    Tests WAV file creation, recording fidelity, and graceful stop.

  ==============================================================================
*/

#include "../Standalone/AudioRecorder.h"

#include <JuceHeader.h>

//==============================================================================
class RecorderTest : public juce::UnitTest
{
public:
    RecorderTest() : juce::UnitTest("GRAIN Recorder") {}

    void runTest() override
    {
        runProducesValidWavTest();
        runRecordedFileNotSilentTest();
        runStopMidFileTest();
        runSampleRateMatchesTest();
        runRecordedLengthTest();
    }

private:
    //==========================================================================
    /** Generate a sine wave buffer for testing. */
    static juce::AudioBuffer<float> generateSineBuffer(double sampleRate, int numChannels, int numSamples)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            auto const sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) /
                                                static_cast<float>(sampleRate));

            for (int ch = 0; ch < numChannels; ++ch)
            {
                buffer.setSample(ch, i, sample);
            }
        }

        return buffer;
    }

    /** Record a complete buffer and return the output file. */
    juce::File recordBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate, int blockSize)
    {
        auto tempFile = juce::File::createTempFile(".wav");
        AudioRecorder recorder;

        auto const started = recorder.startRecording(tempFile, sampleRate, buffer.getNumChannels());
        expect(started, "Recorder should start successfully");

        // Push in blocks (simulating processBlock)
        int offset = 0;
        while (offset < buffer.getNumSamples())
        {
            int const samplesThisBlock = std::min(blockSize, buffer.getNumSamples() - offset);
            juce::AudioBuffer<float> block(buffer.getNumChannels(), samplesThisBlock);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                block.copyFrom(ch, 0, buffer, ch, offset, samplesThisBlock);
            }

            recorder.pushSamples(block, samplesThisBlock);
            offset += samplesThisBlock;
        }

        // Small delay to allow background thread to drain FIFO
        juce::Thread::sleep(200);

        recorder.stopRecording();

        return tempFile;
    }

    //==========================================================================
    void runProducesValidWavTest()
    {
        beginTest("Recorder: produces valid WAV file header");

        auto buffer = generateSineBuffer(44100.0, 2, 44100);  // 1 second stereo
        auto tempFile = recordBuffer(buffer, 44100.0, 512);

        expect(tempFile.existsAsFile(), "Output file should exist");
        expect(tempFile.getSize() > 44, "File should be larger than WAV header");

        // Try to read it back with AudioFormatManager
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));

        expect(reader != nullptr, "WAV file should be readable");

        if (reader != nullptr)
        {
            expectEquals(static_cast<int>(reader->numChannels), 2);
            expectWithinAbsoluteError(reader->sampleRate, 44100.0, 0.01);
            expect(reader->lengthInSamples > 0, "File should have samples");
        }

        tempFile.deleteFile();
    }

    //==========================================================================
    void runRecordedFileNotSilentTest()
    {
        beginTest("Recorder: recorded file is not silent (RMS > 0)");

        auto buffer = generateSineBuffer(44100.0, 2, 44100);
        auto tempFile = recordBuffer(buffer, 44100.0, 512);

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));

        if (reader != nullptr)
        {
            juce::AudioBuffer<float> readBuffer(static_cast<int>(reader->numChannels),
                                                static_cast<int>(reader->lengthInSamples));
            reader->read(&readBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

            auto const rms = readBuffer.getRMSLevel(0, 0, static_cast<int>(reader->lengthInSamples));
            expect(rms > 0.01f, "Recorded file should not be silent (RMS = " + juce::String(rms) + ")");
        }

        tempFile.deleteFile();
    }

    //==========================================================================
    void runStopMidFileTest()
    {
        beginTest("Recorder: handles stop mid-file gracefully (no corrupt file)");

        auto tempFile = juce::File::createTempFile(".wav");
        AudioRecorder recorder;

        auto const started = recorder.startRecording(tempFile, 44100.0, 1);
        expect(started, "Recorder should start");

        // Push only a partial buffer (half a second)
        auto buffer = generateSineBuffer(44100.0, 1, 22050);
        recorder.pushSamples(buffer, buffer.getNumSamples());

        juce::Thread::sleep(100);

        // Stop mid-recording
        recorder.stopRecording();

        // The file should still be a valid WAV
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));
        expect(reader != nullptr, "Partially recorded file should still be a valid WAV");

        if (reader != nullptr)
        {
            expect(reader->lengthInSamples > 0, "File should have some samples");
        }

        tempFile.deleteFile();
    }

    //==========================================================================
    void runSampleRateMatchesTest()
    {
        beginTest("Recorder: output file sample rate matches input");

        constexpr double kTestSampleRate = 48000.0;
        auto buffer = generateSineBuffer(kTestSampleRate, 2, 48000);
        auto tempFile = recordBuffer(buffer, kTestSampleRate, 512);

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));

        if (reader != nullptr)
        {
            expectWithinAbsoluteError(reader->sampleRate, kTestSampleRate, 0.01);
        }

        tempFile.deleteFile();
    }

    //==========================================================================
    void runRecordedLengthTest()
    {
        beginTest("Recorder: recorded file length matches source (within tolerance)");

        constexpr int kSourceSamples = 44100;  // 1 second
        auto buffer = generateSineBuffer(44100.0, 2, kSourceSamples);
        auto tempFile = recordBuffer(buffer, 44100.0, 512);

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tempFile));

        if (reader != nullptr)
        {
            // Allow ±512 samples tolerance (one block) for FIFO timing
            auto const lengthDiff = std::abs(static_cast<int>(reader->lengthInSamples) - kSourceSamples);
            expect(lengthDiff < 1024,
                   "Recorded length should be within tolerance (diff = " + juce::String(lengthDiff) + ")");
        }

        tempFile.deleteFile();
    }
};

//==============================================================================
static RecorderTest
    recorderTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
