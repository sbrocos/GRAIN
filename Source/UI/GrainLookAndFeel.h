/*
  ==============================================================================

    GrainLookAndFeel.h
    GRAIN — Custom LookAndFeel with dot-style rotary knobs.
    Based on the approved JSX mockup.

  ==============================================================================
*/

#pragma once

#include "../GrainColours.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Custom LookAndFeel for the GRAIN plugin.
 *
 * Provides dot-arc style rotary sliders with three size tiers:
 *   - Large  (width >= 140): GRAIN, WARM — 35 dots, 120px knob
 *   - Medium (width >= 100): MIX          — 24 dots, 80px knob
 *   - Small  (width <  100): INPUT, OUTPUT — 21 dots, 60px knob
 */
class GrainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrainLookAndFeel();
    ~GrainLookAndFeel() override = default;

    //==============================================================================
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    //==============================================================================
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    //==============================================================================
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    //==============================================================================
    /** Returns the font used for slider value text (Roboto for numbers). */
    [[nodiscard]] static juce::Font getSliderFont();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainLookAndFeel)
};
