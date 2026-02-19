/*
  ==============================================================================

    BypassControl.h
    GRAIN — LED-style circular bypass button with APVTS attachment.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Circular LED-style bypass toggle button connected to the "bypass" APVTS parameter.
 *
 * Visual appearance is handled entirely by GrainLookAndFeel (componentID = "bypass").
 * Query isBypassed() to apply alpha dimming to other controls.
 */
class BypassControl : public juce::Component
{
public:
    explicit BypassControl(juce::AudioProcessorValueTreeState& apvts);
    ~BypassControl() override = default;

    void resized() override;

    [[nodiscard]] bool isBypassed() const { return bypassButton.getToggleState(); }

private:
    juce::TextButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BypassControl)
};
