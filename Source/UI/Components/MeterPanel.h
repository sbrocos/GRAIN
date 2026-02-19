/*
  ==============================================================================

    MeterPanel.h
    GRAIN — Single-channel segmented LED meter with peak hold.

  ==============================================================================
*/

#pragma once

#include "../../PluginProcessor.h"

#include <JuceHeader.h>

//==============================================================================
/** Peak-hold tracker for meter display. */
struct PeakHold
{
    float peakLevel = 0.0f;
    int holdCounter = 0;

    void update(float newLevel)
    {
        if (newLevel >= peakLevel)
        {
            peakLevel = newLevel;
            holdCounter = 30;  // ~1s hold at 30 FPS
        }
        else if (holdCounter > 0)
        {
            --holdCounter;
        }
        else
        {
            peakLevel *= 0.95f;
        }
    }

    void reset()
    {
        peakLevel = 0.0f;
        holdCounter = 0;
    }
};

//==============================================================================
/**
 * Single stereo LED meter (L+R channels) with label, peak hold, and decay smoothing.
 *
 * Reads audio levels from two atomic<float> members provided at construction.
 * Renders 32 LED segments per channel with green/yellow/red thresholds and glow.
 *
 * Call updateLevels() at 30 FPS from the editor's timerCallback().
 * Place one instance for IN and one for OUT via addAndMakeVisible().
 */
class MeterPanel : public juce::Component
{
public:
    /**
     * @param levelL   Atomic float for the left channel level (gain, not dB).
     * @param levelR   Atomic float for the right channel level.
     * @param label    Label text displayed above the meter ("IN" or "OUT").
     */
    MeterPanel(std::atomic<float>& levelL, std::atomic<float>& levelR, const juce::String& label);
    ~MeterPanel() override = default;

    /** Pull fresh levels and trigger repaint. Call from a 30 FPS timer. */
    void updateLevels();

    void paint(juce::Graphics& g) override;

private:
    // NOLINTNEXTLINE(readability-function-size) — segmented meter rendering with glow + peak hold
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds) const;

    std::atomic<float>& atomicL;
    std::atomic<float>& atomicR;
    juce::String meterLabel;

    // Smoothed display levels
    float displayL = 0.0f, displayR = 0.0f;
    static constexpr float kMeterDecay = 0.85f;

    PeakHold peakHoldL, peakHoldR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterPanel)
};
