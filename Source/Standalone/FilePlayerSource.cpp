/*
  ==============================================================================

    FilePlayerSource.cpp
    GRAIN — Standalone audio file loader implementation (GT-15).

  ==============================================================================
*/

#include "FilePlayerSource.h"

//==============================================================================
FilePlayerSource::FilePlayerSource()
{
    formatManager.registerBasicFormats();
    backgroundThread.startThread(juce::Thread::Priority::normal);
}

FilePlayerSource::~FilePlayerSource()
{
    transportSource.setSource(nullptr);
    readerSource.reset();
    currentReader.reset();
    backgroundThread.stopThread(2000);
}

//==============================================================================
bool FilePlayerSource::loadFile(const juce::File& file)
{
    unloadFile();

    if (!file.existsAsFile())
    {
        lastError = "File not found: " + file.getFullPathName();
        return false;
    }

    auto* reader = formatManager.createReaderFor(file);

    if (reader == nullptr)
    {
        lastError = "Unsupported or invalid audio file: " + file.getFileName();
        return false;
    }

    if (reader->numChannels < 1 || reader->numChannels > 2)
    {
        lastError = "Unsupported channel count: " + juce::String(reader->numChannels) + " (expected mono or stereo)";
        delete reader;
        return false;
    }

    if (reader->lengthInSamples <= 0)
    {
        lastError = "File contains no audio data: " + file.getFileName();
        delete reader;
        return false;
    }

    // Cache metadata
    fileSampleRate = reader->sampleRate;
    fileLengthInSamples = reader->lengthInSamples;
    fileNumChannels = static_cast<int>(reader->numChannels);

    // Build source chain: reader -> readerSource -> transportSource
    currentReader.reset(reader);
    readerSource = std::make_unique<juce::AudioFormatReaderSource>(currentReader.get(), false);

    transportSource.setSource(readerSource.get(), 32768, &backgroundThread, fileSampleRate);

    // Generate thumbnail asynchronously
    thumbnail.setSource(new juce::FileInputSource(file));

    loadedFile = file;
    fileLoaded = true;
    lastError.clear();

    return true;
}

void FilePlayerSource::unloadFile()
{
    transportSource.setSource(nullptr);
    readerSource.reset();
    currentReader.reset();
    thumbnail.setSource(nullptr);

    loadedFile = juce::File();
    fileSampleRate = 0.0;
    fileLengthInSamples = 0;
    fileNumChannels = 0;
    fileLoaded = false;
}

//==============================================================================
bool FilePlayerSource::isFileLoaded() const
{
    return fileLoaded;
}

juce::String FilePlayerSource::getLastError() const
{
    return lastError;
}

//==============================================================================
double FilePlayerSource::getFileSampleRate() const
{
    return fileSampleRate;
}

juce::int64 FilePlayerSource::getFileLengthInSamples() const
{
    return fileLengthInSamples;
}

double FilePlayerSource::getFileDurationSeconds() const
{
    if (fileSampleRate > 0.0)
    {
        return static_cast<double>(fileLengthInSamples) / fileSampleRate;
    }

    return 0.0;
}

int FilePlayerSource::getFileNumChannels() const
{
    return fileNumChannels;
}

juce::File FilePlayerSource::getLoadedFile() const
{
    return loadedFile;
}

//==============================================================================
juce::AudioThumbnail& FilePlayerSource::getThumbnail()
{
    return thumbnail;
}

bool FilePlayerSource::isThumbnailReady() const
{
    return thumbnail.isFullyLoaded();
}

//==============================================================================
void FilePlayerSource::prepareToPlay(double deviceSampleRate, int maxBlockSize)
{
    transportSource.prepareToPlay(maxBlockSize, deviceSampleRate);
}

void FilePlayerSource::releaseResources()
{
    transportSource.releaseResources();
}

juce::AudioTransportSource* FilePlayerSource::getTransportSource()
{
    if (fileLoaded)
    {
        return &transportSource;
    }

    return nullptr;
}
