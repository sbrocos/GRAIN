/*
  ==============================================================================

    StandalonePanel.h
    GRAIN — Standalone-only panel: waveform display + transport bar,
    owning the file player, recorder, and managing the export workflow.

  ==============================================================================
*/

#pragma once

#include "../../PluginProcessor.h"
#include "../../Standalone/AudioFileUtils.h"
#include "../../Standalone/AudioRecorder.h"
#include "../../Standalone/FilePlayerSource.h"
#include "../../Standalone/TransportBar.h"
#include "../../Standalone/WaveformDisplay.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Self-contained standalone panel (170px: 120px waveform + 50px transport bar).
 *
 * Owns and manages:
 *   - FilePlayerSource (audio file loading + playback)
 *   - WaveformDisplay (dry + wet waveform overlay)
 *   - TransportBar (open/play/stop/loop/export controls)
 *   - AudioRecorder (lock-free WAV export)
 *
 * Implements TransportBar::Listener and FilePlayerSource::Listener.
 * Coordinates the export workflow internally (rewind → play → record → stop).
 *
 * Call loadFile() externally (e.g. from drag & drop) to load a file without
 * going through the file chooser dialog.
 */
class StandalonePanel : public juce::Component, public TransportBar::Listener, public FilePlayerSource::Listener
{
public:
    explicit StandalonePanel(GRAINAudioProcessor& processor);
    ~StandalonePanel() override;

    void resized() override;

    /** Load a file into the player (e.g. from drag & drop). */
    void loadFile(const juce::File& file);

    /** Returns true if the file player has a file loaded. */
    [[nodiscard]] bool isFileLoaded() const;

    /** Exposes the underlying FilePlayerSource for drag & drop file validation. */
    [[nodiscard]] FilePlayerSource& getFilePlayer() { return *filePlayer; }

private:
    // TransportBar::Listener
    void openFileRequested() override;
    void stopRequested() override;
    void exportRequested() override;

    // FilePlayerSource::Listener
    void transportStateChanged(bool isNowPlaying) override;
    void transportReachedEnd() override;

    GRAINAudioProcessor& processor;

    std::unique_ptr<FilePlayerSource> filePlayer;
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<AudioRecorder> recorder;

    // File chooser must persist across async dialog lifetime
    std::unique_ptr<juce::FileChooser> fileChooser;

    bool exporting = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandalonePanel)
};
