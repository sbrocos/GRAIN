/*
  ==============================================================================

    ControlPanel.h
    GRAIN — Main parameter controls: large knobs (GRAIN, WARM),
    small knobs (INPUT, MIX, OUTPUT), and 3-way FOCUS selector.

  ==============================================================================
*/

#pragma once

#include "../../GrainColours.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Self-contained control panel managing all plugin parameters:
 *
 *   - Large knobs: GRAIN (drive) and WARM (warmth)
 *   - Small knobs: INPUT, MIX, OUTPUT
 *   - FOCUS selector: LOW / MID / HIGH buttons (manual radio behavior)
 *
 * Owns all APVTS attachments and listens for the "focus" parameter change
 * to stay in sync with DAW automation.
 *
 * Visual appearance (knob dot-arc style, focus button rounded rect) is
 * delegated to GrainLookAndFeel via the parent's LookAndFeel.
 */
class ControlPanel : public juce::Component, public juce::AudioProcessorValueTreeState::Listener
{
public:
    ControlPanel(juce::AudioProcessorValueTreeState& apvts, juce::AudioProcessorValueTreeState::Listener* /*unused*/);
    explicit ControlPanel(juce::AudioProcessorValueTreeState& apvts);
    ~ControlPanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Update control alpha to reflect bypass state. */
    void setBypassAlpha(float alpha);

    // Exposed for testing
    [[nodiscard]] bool isFocusLowActive() const { return focusLowButton.getToggleState(); }
    [[nodiscard]] bool isFocusMidActive() const { return focusMidButton.getToggleState(); }
    [[nodiscard]] bool isFocusHighActive() const { return focusHighButton.getToggleState(); }

private:
    // APVTS::Listener — called from audio thread, dispatches to message thread
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void updateFocusButtons(int selectedIndex);
    void setupRotarySlider(juce::Slider& slider, int textBoxWidth, int textBoxHeight,
                           const juce::String& suffix = {});
    void setupLabel(juce::Label& label);

    juce::AudioProcessorValueTreeState& apvts;

    // Large knobs (creative controls)
    juce::Slider grainSlider;
    juce::Slider warmthSlider;
    juce::Label grainLabel{{}, "GRAIN"};
    juce::Label warmthLabel{{}, "WARM"};

    // Small knobs (utility controls)
    juce::Slider inputSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;
    juce::Label inputLabel{{}, "INPUT"};
    juce::Label mixLabel{{}, "MIX"};
    juce::Label outputLabel{{}, "OUTPUT"};

    // Focus selector
    juce::TextButton focusLowButton{"LOW"};
    juce::TextButton focusMidButton{"MID"};
    juce::TextButton focusHighButton{"HIGH"};
    juce::Label focusLabel{{}, "FOCUS"};

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};
