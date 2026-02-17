/*
  ==============================================================================

    TransportBar.h
    GRAIN — Standalone transport bar UI component (GT-17).
    Provides Open, Play/Stop, Loop, Export buttons, time display, and
    a clickable progress bar for file playback control.

  ==============================================================================
*/

#pragma once

#include "FilePlayerSource.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Transport bar component for the GRAIN standalone application.
 *
 * Layout (50px height):
 *   [Open] [■] [▶/⏸] [↻] [Export]   00:12 / 01:45
 *   [═══════════════●══════════════════════════]
 *
 * Connects to a FilePlayerSource for transport control and state.
 * Uses a Timer to update position display and progress bar at 30 FPS.
 */
class TransportBar
    : public juce::Component
    , public FilePlayerSource::Listener
    , private juce::Timer
{
public:
    //==============================================================================
    /** Listener for transport bar actions that require external handling. */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when the user clicks "Open" to load a file. */
        virtual void openFileRequested() = 0;

        /** Called when the user clicks "Stop" (rewind to start). */
        virtual void stopRequested() = 0;

        /** Called when the user clicks "Export" to record processed output. */
        virtual void exportRequested() = 0;
    };

    //==============================================================================
    explicit TransportBar(FilePlayerSource& filePlayer);
    ~TransportBar() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    //==============================================================================
    // FilePlayerSource::Listener
    void transportStateChanged(bool isNowPlaying) override;
    void transportReachedEnd() override;

    //==============================================================================
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    //==============================================================================
    /** Update button states to reflect current transport state. */
    void updateButtonStates();

    /** Format time in seconds to MM:SS string. */
    static juce::String formatTime(double seconds);

private:
    //==============================================================================
    void timerCallback() override;

    /** Get the progress bar bounds within the component. */
    juce::Rectangle<int> getProgressBarBounds() const;

    //==============================================================================
    FilePlayerSource& player;

    // Buttons
    juce::TextButton openButton{"Open"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton playPauseButton{"Play"};
    juce::TextButton loopButton;
    juce::TextButton exportButton{"Export"};

    // Display state
    juce::String timeText;
    float progressNormalized = 0.0f;

    // Listeners
    juce::ListenerList<Listener> listeners;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};
