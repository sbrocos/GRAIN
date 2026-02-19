/*
  ==============================================================================

    HeaderPanel.cpp
    GRAIN — Header component: title + subtitle with embedded Inter fonts.

  ==============================================================================
*/

#include "HeaderPanel.h"

#include "BinaryData.h"

#include "../../GrainColours.h"

//==============================================================================
void HeaderPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Title: Inter ExtraBold Italic 30px, tracking-tight (-0.025em)
    g.setColour(GrainColours::kText);
    {
        static auto titleTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterExtraBoldItalic_ttf, BinaryData::InterExtraBoldItalic_ttfSize);
        auto titleFont = juce::Font(juce::FontOptions(titleTypeface).withPointHeight(30.0f));
        titleFont.setExtraKerningFactor(-0.025f);
        g.setFont(titleFont);
    }
    g.drawText("GRAIN", bounds, juce::Justification::centredLeft);

    // Subtitle: Inter Regular 12px, tracking-widest (0.1em)
    auto subtitleArea = bounds;
    subtitleArea.removeFromTop(34);
    g.setColour(GrainColours::kTextDim);
    {
        static auto subtitleTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
        auto subtitleFont = juce::Font(juce::FontOptions(subtitleTypeface).withPointHeight(12.0f));
        subtitleFont.setExtraKerningFactor(0.1f);
        g.setFont(subtitleFont);
    }
    g.drawText("MICRO-HARMONIC SATURATION", subtitleArea, juce::Justification::centredLeft);
}
