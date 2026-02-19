/*
  ==============================================================================

    FooterPanel.h
    GRAIN — Footer component: separator line + version + copyright.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Renders the plugin footer: a thin separator line, version string on the left,
 * and copyright notice on the right.
 *
 * Self-contained — no external dependencies.
 */
class FooterPanel : public juce::Component
{
public:
    FooterPanel() = default;
    ~FooterPanel() override = default;

    void paint(juce::Graphics& g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterPanel)
};
