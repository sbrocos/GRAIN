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
 *
 * Also provides:
 *   - LED-style bypass button (circular, orange/dark)
 *   - Focus selector buttons (rounded rect, orange active)
 *   - Editable value fields (transparent bg, orange outline on focus)
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
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    //==============================================================================
    // Slider text box (ensures TextEditor matches Label alignment)
    juce::Label* createSliderTextBox(juce::Slider& slider) override;

    //==============================================================================
    // TextEditor styling (for editable slider value fields)
    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override;
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override;

    //==============================================================================
    // Button styling (bypass LED + focus selector)
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainLookAndFeel)
};
