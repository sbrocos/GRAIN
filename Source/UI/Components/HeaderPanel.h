/*
  ==============================================================================

    HeaderPanel.h
    GRAIN — Header component: title + subtitle with embedded Inter fonts.

  ==============================================================================
*/

#pragma once

#include "../../GrainColours.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Renders the plugin header: "GRAIN" title in Inter ExtraBold Italic
 * and "MICRO-HARMONIC SATURATION" subtitle in Inter Regular.
 *
 * Self-contained — no external dependencies beyond BinaryData and GrainColours.
 */
class HeaderPanel : public juce::Component
{
public:
    HeaderPanel() = default;
    ~HeaderPanel() override = default;

    void paint(juce::Graphics& g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderPanel)
};
