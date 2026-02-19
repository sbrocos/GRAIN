/*
  ==============================================================================

    FooterPanel.cpp
    GRAIN — Footer component: separator line + version + copyright.

  ==============================================================================
*/

#include "FooterPanel.h"

#include "../../GrainColours.h"

//==============================================================================
void FooterPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Separator line at top
    g.setColour(juce::Colours::black.withAlpha(0.1f));
    g.drawHorizontalLine(bounds.getY(), static_cast<float>(bounds.getX()),
                         static_cast<float>(bounds.getRight()));

    // Version (left) and copyright (right)
    g.setColour(GrainColours::kTextDim);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("v1.0.0", bounds, juce::Justification::centredLeft);
    g.drawText(juce::CharPointer_UTF8("Sergio Brocos \xc2\xa9 2025"), bounds, juce::Justification::centredRight);
}
