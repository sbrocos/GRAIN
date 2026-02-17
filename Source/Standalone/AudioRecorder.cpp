/*
  ==============================================================================

    AudioRecorder.cpp
    GRAIN — Real-time audio recorder implementation (GT-20).

  ==============================================================================
*/

#include "AudioRecorder.h"

//==============================================================================
AudioRecorder::AudioRecorder()
{
    writerThread.startThread(juce::Thread::Priority::normal);
}

AudioRecorder::~AudioRecorder()
{
    stopRecording();
    writerThread.stopThread(2000);
}

//==============================================================================
bool AudioRecorder::startRecording(const juce::File& outputFile, double sampleRate, int numChannels)
{
    // Stop any existing recording first
    stopRecording();

    numRecordChannels = numChannels;
    fifoBuffer.setSize(numChannels, kFifoSize);
    fifoBuffer.clear();
    fifo.reset();

    // Create WAV writer
    std::unique_ptr<juce::OutputStream> outputStream = outputFile.createOutputStream();

    if (outputStream == nullptr)
    {
        return false;
    }

    juce::WavAudioFormat wavFormat;
    auto options =
        juce::AudioFormatWriterOptions().withSampleRate(sampleRate).withNumChannels(numChannels).withBitsPerSample(24);

    writer = wavFormat.createWriterFor(outputStream, options);

    if (writer == nullptr)
    {
        return false;
    }

    currentFile = outputFile;
    recording.store(true);

    // Register with the background thread for periodic drain
    writerThread.addTimeSliceClient(this);

    return true;
}

void AudioRecorder::stopRecording()
{
    if (!recording.load())
    {
        return;
    }

    recording.store(false);

    // Unregister from background thread
    writerThread.removeTimeSliceClient(this);

    // Flush any remaining samples in the FIFO
    writeFromFifo();

    // Close the writer (finalizes WAV header)
    writer.reset();
    currentFile = juce::File();
}

bool AudioRecorder::isRecording() const
{
    return recording.load();
}

//==============================================================================
void AudioRecorder::pushSamples(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!recording.load() || writer == nullptr)
    {
        return;
    }

    const int available = fifo.getFreeSpace();
    const int toWrite = std::min(numSamples, available);

    if (toWrite <= 0)
    {
        return;
    }

    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    fifo.prepareToWrite(toWrite, start1, size1, start2, size2);

    const int channels = std::min(buffer.getNumChannels(), numRecordChannels);

    // Copy first segment
    if (size1 > 0)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            fifoBuffer.copyFrom(ch, start1, buffer, ch, 0, size1);
        }
    }

    // Copy second segment (wrap-around)
    if (size2 > 0)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            fifoBuffer.copyFrom(ch, start2, buffer, ch, size1, size2);
        }
    }

    fifo.finishedWrite(size1 + size2);
}

juce::File AudioRecorder::getRecordingFile() const
{
    return currentFile;
}

//==============================================================================
int AudioRecorder::useTimeSlice()
{
    writeFromFifo();

    // Return 0 = call again ASAP if there's data, otherwise wait a bit
    return fifo.getNumReady() > 0 ? 0 : 10;
}

void AudioRecorder::writeFromFifo()
{
    if (writer == nullptr)
    {
        return;
    }

    const int numReady = fifo.getNumReady();

    if (numReady <= 0)
    {
        return;
    }

    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    fifo.prepareToRead(numReady, start1, size1, start2, size2);

    // Write first segment
    if (size1 > 0)
    {
        // Create a sub-buffer view for this segment
        const juce::AudioBuffer<float> segment(fifoBuffer.getArrayOfWritePointers(), numRecordChannels, start1, size1);
        writer->writeFromAudioSampleBuffer(segment, 0, size1);
    }

    // Write second segment (wrap-around)
    if (size2 > 0)
    {
        const juce::AudioBuffer<float> segment(fifoBuffer.getArrayOfWritePointers(), numRecordChannels, start2, size2);
        writer->writeFromAudioSampleBuffer(segment, 0, size2);
    }

    fifo.finishedRead(size1 + size2);
}
