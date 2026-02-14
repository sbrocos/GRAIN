/*
  ==============================================================================

    PluginEditor.h
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor: functional layout with controls, meters, and basic styling.
    Phase A — structure and connectivity, not final visual polish.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"

#include <JuceHeader.h>

//==============================================================================
// Basic color palette for Phase A
namespace GrainColours
{
const juce::Colour kBackground{0xff1c1917};   // Dark stone
const juce::Colour kSurface{0xff292524};      // Slightly lighter
const juce::Colour kText{0xffa8a29e};         // Stone 400
const juce::Colour kTextBright{0xfff5f5f4};   // Stone 100
const juce::Colour kAccent{0xffd97706};       // Amber 600
const juce::Colour kAccentDim{0xff92400e};    // Amber 800
const juce::Colour kMeterGreen{0xff22c55e};   // Green 500
const juce::Colour kMeterYellow{0xffeab308};  // Yellow 500
const juce::Colour kMeterRed{0xffef4444};     // Red 500
}  // namespace GrainColours

//==============================================================================
/**
 * Plugin editor for GRAIN (Phase A — functional layout).
 *
 * Layout:
 *   Header:  Title "GRAIN" + Bypass button
 *   Main:    Grain (drive) + Warmth knobs (large), Input/Output meters (L/R)
 *   Footer:  Input Gain + Mix + Focus selector + Output knob (small)
 *
 * Meters use a 30 FPS timer reading atomic levels from the processor.
 */
class GRAINAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit GRAINAudioProcessorEditor(GRAINAudioProcessor& /*p*/);
    ~GRAINAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics& /*g*/) override;
    void resized() override;

private:
    //==============================================================================
    void timerCallback() override;
    static void drawMeter(juce::Graphics& /*g*/, juce::Rectangle<float> area, float levelL, float levelR,
                          const juce::String& label);

    // Setup helpers (reduce constructor boilerplate)
    void setupRotarySlider(juce::Slider& slider, int textBoxWidth, int textBoxHeight, const juce::String& suffix = {});
    void setupLabel(juce::Label& label);

    GRAINAudioProcessor& processor;

    // Main controls (creative — big knobs)
    juce::Slider grainSlider;
    juce::Slider warmthSlider;

    // Secondary controls (utility — small knobs)
    juce::Slider inputSlider;
    juce::Slider mixSlider;
    juce::ComboBox focusSelector;
    juce::Slider outputSlider;

    // Bypass
    juce::TextButton bypassButton{"BYPASS"};

    // Labels
    juce::Label grainLabel{{}, "GRAIN"};
    juce::Label warmthLabel{{}, "WARMTH"};
    juce::Label inputLabel{{}, "INPUT"};
    juce::Label mixLabel{{}, "MIX"};
    juce::Label focusLabel{{}, "FOCUS"};
    juce::Label outputLabel{{}, "OUTPUT"};

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> focusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // Meter display values (smoothed via decay)
    float displayInputL = 0.0f, displayInputR = 0.0f;
    float displayOutputL = 0.0f, displayOutputR = 0.0f;

    static constexpr float kMeterDecay = 0.85f;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRAINAudioProcessorEditor)
};
