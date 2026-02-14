/*
  ==============================================================================

    PluginEditor.cpp
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor implementation: functional layout (Phase A).

  ==============================================================================
*/

#include "PluginEditor.h"

#include "PluginProcessor.h"

namespace
{
constexpr int kEditorWidth = 500;
constexpr int kEditorHeight = 350;
constexpr int kHeaderHeight = 50;
constexpr int kFooterHeight = 100;
constexpr int kMeterColumnWidth = 60;  // meter + padding
}  // namespace

//==============================================================================
void GRAINAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, int textBoxWidth, int textBoxHeight,
                                                  const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, textBoxHeight);

    if (suffix.isNotEmpty())
    {
        slider.setTextValueSuffix(suffix);
    }

    addAndMakeVisible(slider);
}

void GRAINAudioProcessorEditor::setupLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, GrainColours::kText);
    addAndMakeVisible(label);
}

//==============================================================================
GRAINAudioProcessorEditor::GRAINAudioProcessorEditor(GRAINAudioProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setSize(kEditorWidth, kEditorHeight);

    auto& apvts = processor.getAPVTS();

    // === Main controls (creative — large knobs) ===
    setupRotarySlider(grainSlider, 60, 20);
    grainAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "drive", grainSlider);
    setupLabel(grainLabel);

    setupRotarySlider(warmthSlider, 60, 20);
    warmthAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "warmth", warmthSlider);
    setupLabel(warmthLabel);

    // === Secondary controls (utility — small knobs) ===
    setupRotarySlider(inputSlider, 50, 18, " dB");
    inputAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "inputGain", inputSlider);
    setupLabel(inputLabel);

    setupRotarySlider(mixSlider, 50, 18);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mix", mixSlider);
    setupLabel(mixLabel);

    // === Focus (selector) ===
    focusSelector.addItem("LOW", 1);
    focusSelector.addItem("MID", 2);
    focusSelector.addItem("HIGH", 3);
    addAndMakeVisible(focusSelector);
    focusAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "focus", focusSelector);
    setupLabel(focusLabel);

    setupRotarySlider(outputSlider, 50, 18, " dB");
    outputAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "output", outputSlider);
    setupLabel(outputLabel);

    // === Bypass ===
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, GrainColours::kAccent);
    bypassButton.setColour(juce::TextButton::textColourOffId, GrainColours::kText);
    bypassButton.setColour(juce::TextButton::textColourOnId, GrainColours::kTextBright);
    addAndMakeVisible(bypassButton);
    bypassAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "bypass", bypassButton);

    // Start meter timer (30 FPS)
    startTimerHz(30);
}

GRAINAudioProcessorEditor::~GRAINAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void GRAINAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(GrainColours::kBackground);

    // Header area
    const auto headerArea = getLocalBounds().removeFromTop(kHeaderHeight);
    g.setColour(GrainColours::kSurface);
    g.fillRect(headerArea);

    // Title — JUCE 8 Font API
    g.setColour(GrainColours::kTextBright);
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    g.drawText("GRAIN", headerArea.reduced(15, 0), juce::Justification::centredLeft);

    // Meters
    auto bounds = getLocalBounds();
    bounds.removeFromTop(kHeaderHeight);
    bounds.removeFromBottom(kFooterHeight);

    const auto inputMeterArea = bounds.removeFromLeft(kMeterColumnWidth).toFloat().reduced(10, 20);
    const auto outputMeterArea = bounds.removeFromRight(kMeterColumnWidth).toFloat().reduced(10, 20);

    drawMeter(g, inputMeterArea, displayInputL, displayInputR, "IN");
    drawMeter(g, outputMeterArea, displayOutputL, displayOutputR, "OUT");

    // Footer separator
    g.setColour(GrainColours::kSurface);
    g.fillRect(getLocalBounds().removeFromBottom(kFooterHeight));
}

void GRAINAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> area, float levelL, float levelR,
                                          const juce::String& label)
{
    const float labelHeight = 20.0f;
    const auto labelArea = area.removeFromTop(labelHeight);

    g.setColour(GrainColours::kText);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(label, labelArea, juce::Justification::centred);

    auto meterArea = area.reduced(2, 0);
    const auto meterBarWidth = (meterArea.getWidth() / 2.0f) - 2.0f;

    const auto leftArea = meterArea.removeFromLeft(meterBarWidth);
    meterArea.removeFromLeft(4);  // Gap between L/R
    const auto rightArea = meterArea;

    // Meter backgrounds
    g.setColour(juce::Colours::black);
    g.fillRect(leftArea);
    g.fillRect(rightArea);

    // Draw level bars
    auto drawLevel = [&](juce::Rectangle<float> meterRect, float level)
    {
        float const dbLevel = juce::Decibels::gainToDecibels(level, -60.0f);
        float normalized = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        auto levelHeight = meterRect.getHeight() * normalized;
        auto levelRect = meterRect.removeFromBottom(levelHeight);

        // Color: green → yellow → red
        if (normalized < 0.7f)
        {
            g.setColour(GrainColours::kMeterGreen);
        }
        else if (normalized < 0.9f)
        {
            g.setColour(GrainColours::kMeterYellow);
        }
        else
        {
            g.setColour(GrainColours::kMeterRed);
        }

        g.fillRect(levelRect);
    };

    drawLevel(leftArea.reduced(1), levelL);
    drawLevel(rightArea.reduced(1), levelR);
}

//==============================================================================
void GRAINAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header
    auto headerArea = bounds.removeFromTop(kHeaderHeight);
    bypassButton.setBounds(headerArea.removeFromRight(80).reduced(10));

    // Footer (secondary controls — 4 items: INPUT | MIX | FOCUS | OUTPUT)
    auto footerArea = bounds.removeFromBottom(kFooterHeight);
    auto footerControls = footerArea.reduced(20, 10);

    auto controlWidth = footerControls.getWidth() / 4;

    auto inputArea = footerControls.removeFromLeft(controlWidth);
    inputLabel.setBounds(inputArea.removeFromTop(20));
    inputSlider.setBounds(inputArea);

    auto mixArea = footerControls.removeFromLeft(controlWidth);
    mixLabel.setBounds(mixArea.removeFromTop(20));
    mixSlider.setBounds(mixArea);

    auto focusArea = footerControls.removeFromLeft(controlWidth);
    focusLabel.setBounds(focusArea.removeFromTop(20));
    focusSelector.setBounds(focusArea.reduced(10, 20));

    auto outputArea = footerControls;
    outputLabel.setBounds(outputArea.removeFromTop(20));
    outputSlider.setBounds(outputArea);

    // Main area (big knobs — creative controls: GRAIN + WARMTH)
    auto mainArea = bounds;
    mainArea.removeFromLeft(kMeterColumnWidth);
    mainArea.removeFromRight(kMeterColumnWidth);
    mainArea.reduce(10, 20);

    auto knobWidth = mainArea.getWidth() / 2;

    auto grainArea = mainArea.removeFromLeft(knobWidth);
    grainLabel.setBounds(grainArea.removeFromTop(25));
    grainSlider.setBounds(grainArea);

    auto warmthArea = mainArea;
    warmthLabel.setBounds(warmthArea.removeFromTop(25));
    warmthSlider.setBounds(warmthArea);
}

//==============================================================================
void GRAINAudioProcessorEditor::timerCallback()
{
    // Smooth meter decay
    // NOTE: Phase A repaints entire editor at 30 FPS for simplicity.
    // Phase B should optimize to repaint only meter areas.
    float const inL = processor.inputLevelL.load();
    float const inR = processor.inputLevelR.load();
    float const outL = processor.outputLevelL.load();
    float const outR = processor.outputLevelR.load();

    displayInputL = std::max(inL, displayInputL * kMeterDecay);
    displayInputR = std::max(inR, displayInputR * kMeterDecay);
    displayOutputL = std::max(outL, displayOutputL * kMeterDecay);
    displayOutputR = std::max(outR, displayOutputR * kMeterDecay);

    repaint();
}
