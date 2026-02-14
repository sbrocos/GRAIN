/*
  ==============================================================================

    FilePlayerSource.h
    GRAIN — Standalone audio file loader (GT-15).
    Loads WAV/AIFF files, validates format, handles sample rate mismatch,
    and generates AudioThumbnail for waveform display.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Loads audio files (WAV/AIFF) and prepares them for playback.
 *
 * Owns the AudioFormatManager, AudioFormatReaderSource, AudioTransportSource,
 * AudioThumbnail, and background thread. Designed for extension by subtask 2
 * (transport controls: play/stop/loop/seek).
 *
 * Thread safety:
 *   - loadFile() / unloadFile() must be called from the message thread.
 *   - Metadata getters are safe after loadFile() completes.
 *   - AudioThumbnail is internally thread-safe (JUCE design).
 */
class FilePlayerSource
{
public:
    //==============================================================================
    FilePlayerSource();
    ~FilePlayerSource();

    //==============================================================================
    // File loading (message thread only)

    /** Load an audio file (WAV or AIFF).
     *  @param file The audio file to load.
     *  @return true if loaded successfully, false on error. */
    bool loadFile(const juce::File& file);

    /** Unload the current file and reset to empty state. */
    void unloadFile();

    //==============================================================================
    // State queries

    bool isFileLoaded() const;
    juce::String getLastError() const;

    //==============================================================================
    // File metadata (valid only when isFileLoaded() == true)

    double getFileSampleRate() const;
    juce::int64 getFileLengthInSamples() const;
    double getFileDurationSeconds() const;
    int getFileNumChannels() const;
    juce::File getLoadedFile() const;

    //==============================================================================
    // Thumbnail access (for waveform display)

    juce::AudioThumbnail& getThumbnail();
    bool isThumbnailReady() const;

    //==============================================================================
    // Playback preparation (used by transport in subtask 2)

    /** Prepare the transport source for playback at the given device sample rate.
     *  Handles resampling automatically if file SR differs from device SR. */
    void prepareToPlay(double deviceSampleRate, int maxBlockSize);
    void releaseResources();

    /** Returns the transport source, or nullptr if no file loaded. */
    juce::AudioTransportSource* getTransportSource();

private:
    //==============================================================================
    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread backgroundThread{"GRAIN File Reader"};

    // Audio source chain: reader -> readerSource -> transportSource
    std::unique_ptr<juce::AudioFormatReader> currentReader;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    // Thumbnail for waveform display
    juce::AudioThumbnailCache thumbnailCache{5};
    juce::AudioThumbnail thumbnail{512, formatManager, thumbnailCache};

    // Cached file metadata
    juce::File loadedFile;
    double fileSampleRate = 0.0;
    juce::int64 fileLengthInSamples = 0;
    int fileNumChannels = 0;
    bool fileLoaded = false;

    // Error state
    juce::String lastError;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilePlayerSource)
};
