/*
  ==============================================================================

    AudioRecorder.h
    GRAIN — Real-time audio recorder for standalone export (GT-20).
    Writes processed output samples to a WAV file using a background thread
    for lock-free disk I/O from the audio thread.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Records audio to a WAV file in real-time from the audio thread.
 *
 * Uses an AbstractFifo + background TimeSliceThread for lock-free writing:
 *   - Audio thread pushes samples into a ring buffer (lock-free)
 *   - Background thread drains the ring buffer and writes to disk
 *
 * Thread safety:
 *   - startRecording() / stopRecording() must be called from the message thread.
 *   - pushSamples() is called from the audio thread (lock-free).
 *   - isRecording() is thread-safe (atomic).
 *
 * Usage:
 *   1. Call startRecording(outputFile, sampleRate, numChannels)
 *   2. Push audio blocks via pushSamples() from processBlock
 *   3. Call stopRecording() when done — flushes and closes the file
 */
class AudioRecorder : private juce::TimeSliceClient
{
public:
    //==============================================================================
    AudioRecorder();
    ~AudioRecorder() override;

    //==============================================================================
    /** Start recording to the specified WAV file.
     *  @param outputFile   Destination file (will be created/overwritten).
     *  @param sampleRate   Recording sample rate.
     *  @param numChannels  Number of channels (1 or 2).
     *  @return true if recording started successfully. */
    bool startRecording(const juce::File& outputFile, double sampleRate, int numChannels);

    /** Stop recording, flush remaining samples, and close the file.
     *  Safe to call even if not currently recording. */
    void stopRecording();

    /** @return true if currently recording. Thread-safe. */
    bool isRecording() const;

    //==============================================================================
    /** Push processed audio samples for recording.
     *  Called from the audio thread. Lock-free via ring buffer.
     *  @param buffer  The audio buffer to record.
     *  @param numSamples Number of samples to write from the buffer. */
    void pushSamples(const juce::AudioBuffer<float>& buffer, int numSamples);

    /** @return the file currently being recorded to (empty if not recording). */
    juce::File getRecordingFile() const;

private:
    //==============================================================================
    // TimeSliceClient — runs on background thread
    int useTimeSlice() override;

    /** Write samples from the FIFO to the WAV file. */
    void writeFromFifo();

    //==============================================================================
    juce::TimeSliceThread writerThread{"GRAIN Recorder"};

    // Writer
    std::unique_ptr<juce::AudioFormatWriter> writer;
    juce::File currentFile;

    // Ring buffer for lock-free audio thread → disk thread
    static constexpr int kFifoSize = 65536;  // ~1.5s at 44100 stereo
    juce::AbstractFifo fifo{kFifoSize};
    juce::AudioBuffer<float> fifoBuffer;
    int numRecordChannels = 0;

    // State
    std::atomic<bool> recording{false};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};
