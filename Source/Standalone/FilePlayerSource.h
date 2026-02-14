/*
  ==============================================================================

    FilePlayerSource.h
    GRAIN — Standalone audio file loader and transport (GT-15, GT-16).
    Loads WAV/AIFF files, validates format, handles sample rate mismatch,
    generates AudioThumbnail, and provides transport controls (play/stop/loop/seek).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Loads audio files (WAV/AIFF) and provides transport controls for playback.
 *
 * Owns the AudioFormatManager, AudioFormatReaderSource, AudioTransportSource,
 * AudioThumbnail, and background thread. The transport methods (play/stop/loop/seek)
 * control playback state, and getNextAudioBlock() fills audio buffers that
 * replace device input in the processor's processBlock.
 *
 * Thread safety:
 *   - loadFile() / unloadFile() must be called from the message thread.
 *   - play() / stop() / seekToPosition() are message-thread only.
 *   - isPlaying() / isLooping() / getCurrentPosition() are thread-safe.
 *   - getNextAudioBlock() is called from the audio thread.
 *   - Metadata getters are safe after loadFile() completes.
 *   - AudioThumbnail is internally thread-safe (JUCE design).
 */
class FilePlayerSource : public juce::ChangeListener
{
public:
    //==============================================================================
    /** Listener interface for transport state changes. */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when playback starts or stops. */
        virtual void transportStateChanged(bool isNowPlaying) = 0;

        /** Called when playback reaches the end (with or without loop). */
        virtual void transportReachedEnd() = 0;
    };

    //==============================================================================
    FilePlayerSource();
    ~FilePlayerSource() override;

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
    // Transport controls (message thread only, except isPlaying/isLooping/getCurrentPosition)

    /** Start playback from the current position. No-op if no file loaded. */
    void play();

    /** Stop playback (pauses at current position). No-op if not playing. */
    void stop();

    /** Toggle loop mode on/off. */
    void setLooping(bool shouldLoop);

    /** @return true if loop mode is active. Thread-safe. */
    bool isLooping() const;

    /** @return true if transport is currently playing. Thread-safe. */
    bool isPlaying() const;

    /** Seek to the specified position in seconds. Clamps to [0, duration].
     *  @param positionSeconds Target position in seconds. */
    void seekToPosition(double positionSeconds);

    /** @return Current playback position in seconds. Thread-safe. */
    double getCurrentPosition() const;

    //==============================================================================
    // Audio thread interface

    /** Prepare the transport source for playback at the given device sample rate.
     *  Handles resampling automatically if file SR differs from device SR. */
    void prepareToPlay(double deviceSampleRate, int maxBlockSize);
    void releaseResources();

    /** Fill the given audio buffer with file audio.
     *  Called from the audio thread during processBlock.
     *  @param bufferToFill The audio source channel info to fill. */
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);

    /** Returns the transport source, or nullptr if no file loaded. */
    juce::AudioTransportSource* getTransportSource();

    //==============================================================================
    // Listener management

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    //==============================================================================
    // ChangeListener callback from AudioTransportSource
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

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

    // Transport state
    std::atomic<bool> looping{false};

    // Error state
    juce::String lastError;

    // Listeners
    juce::ListenerList<Listener> listeners;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilePlayerSource)
};
