/*
  ==============================================================================

    WaveformDisplay.h
    GRAIN — Standalone waveform display with dry + wet overlay (GT-18).
    Shows the original waveform (from AudioThumbnail) and the processed
    output waveform superimposed with distinct colors. Supports click-to-seek
    and a playback cursor.

  ==============================================================================
*/

#pragma once

#include "FilePlayerSource.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Waveform display component for the GRAIN standalone application.
 *
 * Renders two overlaid waveforms:
 *   - Dry (pre-processed): drawn from AudioThumbnail, semi-transparent
 *   - Wet (post-processed): accumulated in real-time from processed output
 *
 * Features:
 *   - Click-to-seek (sends position to FilePlayerSource)
 *   - Playback cursor (vertical line at current position)
 *   - Full file always visible (no zoom/scroll for V1)
 *
 * The wet waveform is accumulated via pushWetSamples() called from the
 * editor's timer callback with processed output data.
 */
class WaveformDisplay
    : public juce::Component
    , public FilePlayerSource::Listener
    , private juce::Timer
{
public:
    //==============================================================================
    explicit WaveformDisplay(FilePlayerSource& filePlayer);
    ~WaveformDisplay() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    //==============================================================================
    // FilePlayerSource::Listener
    void transportStateChanged(bool isNowPlaying) override;
    void transportReachedEnd() override;

    //==============================================================================
    /** Push processed (wet) output samples for real-time waveform accumulation.
     *  Called from the audio thread via the processor. Thread-safe (uses FIFO). */
    void pushWetSamples(const float* samples, int numSamples);

    /** Clear the wet waveform buffer (e.g., when a new file is loaded). */
    void clearWetBuffer();

    /** @return true if wet waveform data has been accumulated. */
    bool hasWetData() const;

    //==============================================================================
    /** Map a pixel X position to normalized [0,1] file position. */
    float pixelToNormalized(int pixelX) const;

    /** Map a normalized [0,1] position to pixel X within the waveform area. */
    int normalizedToPixel(float normalized) const;

    /** Get the waveform drawing area (excluding padding). */
    juce::Rectangle<int> getWaveformBounds() const;

private:
    //==============================================================================
    void timerCallback() override;

    /** Draw the dry waveform from AudioThumbnail. */
    void drawDryWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);

    /** Draw the wet waveform from accumulated samples. */
    void drawWetWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);

    /** Draw the playback cursor. */
    void drawCursor(juce::Graphics& g, juce::Rectangle<int> bounds);

    //==============================================================================
    FilePlayerSource& player;

    // Wet waveform accumulation
    // We store min/max pairs per display column for efficient rendering.
    // The buffer is resized when the component width changes.
    struct WetColumn
    {
        float minVal = 0.0f;
        float maxVal = 0.0f;
        int sampleCount = 0;
    };

    std::vector<WetColumn> wetColumns;
    juce::AbstractFifo wetFifo{2048};
    std::vector<float> wetFifoBuffer;
    int wetSampleRate = 44100;
    int wetTotalSamples = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
