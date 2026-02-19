/*
  ==============================================================================

    BypassControl.cpp
    GRAIN — LED-style circular bypass button with APVTS attachment.

  ==============================================================================
*/

#include "BypassControl.h"

//==============================================================================
BypassControl::BypassControl(juce::AudioProcessorValueTreeState& apvts)
{
    bypassButton.setComponentID("bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setButtonText("");
    addAndMakeVisible(bypassButton);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "bypass", bypassButton);
}

//==============================================================================
void BypassControl::resized()
{
    bypassButton.setBounds(getLocalBounds());
}
