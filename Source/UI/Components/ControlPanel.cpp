/*
  ==============================================================================

    ControlPanel.cpp
    GRAIN — Main parameter controls: large knobs (GRAIN, WARM),
    small knobs (INPUT, MIX, OUTPUT), and 3-way FOCUS selector.

  ==============================================================================
*/

#include "ControlPanel.h"

namespace
{
constexpr int kFocusButtonWidth = 52;
constexpr int kFocusButtonHeight = 28;
constexpr int kFocusGap = 2;
constexpr int kFocusContainerPadding = 3;
constexpr int kBottomRowHeight = 100;
constexpr int kSmallKnobContainerWidth = 100;
}  // namespace

//==============================================================================
ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState& a) : apvts(a)
{
    // Large knobs
    setupRotarySlider(grainSlider, 60, 20);
    grainAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "drive", grainSlider);
    setupLabel(grainLabel);

    setupRotarySlider(warmthSlider, 60, 20);
    warmthAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "warmth", warmthSlider);
    setupLabel(warmthLabel);

    // Small knobs
    setupRotarySlider(inputSlider, 50, 18, " dB");
    inputAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "inputGain", inputSlider);
    setupLabel(inputLabel);

    setupRotarySlider(mixSlider, 50, 18);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mix", mixSlider);
    setupLabel(mixLabel);

    setupRotarySlider(outputSlider, 50, 18, " dB");
    outputAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "output", outputSlider);
    setupLabel(outputLabel);

    // Focus buttons (manual radio behavior — radioGroupId causes ordering issues with APVTS)
    auto setupFocusButton = [this](juce::TextButton& btn, int index)
    {
        btn.setComponentID("focusButton");
        btn.setClickingTogglesState(false);
        addAndMakeVisible(btn);

        btn.onClick = [this, index]()
        {
            auto* param = apvts.getParameter("focus");
            param->setValueNotifyingHost(static_cast<float>(index) / 2.0f);
            updateFocusButtons(index);
        };
    };

    setupFocusButton(focusLowButton, 0);
    setupFocusButton(focusMidButton, 1);
    setupFocusButton(focusHighButton, 2);

    // Sync initial state from parameter
    auto* focusParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("focus"));
    const int currentIndex = (focusParam != nullptr) ? focusParam->getIndex() : 1;
    updateFocusButtons(currentIndex);

    // Listen for DAW automation changes
    apvts.addParameterListener("focus", this);

    setupLabel(focusLabel);
}

// Second constructor (unused parameter variant kept for potential external use)
ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState& a,
                           juce::AudioProcessorValueTreeState::Listener* /*unused*/)
    : ControlPanel(a)
{
}

ControlPanel::~ControlPanel()
{
    apvts.removeParameterListener("focus", this);
}

//==============================================================================
void ControlPanel::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "focus")
    {
        const int index = juce::roundToInt(newValue * 2.0f);
        juce::MessageManager::callAsync([this, index]() { updateFocusButtons(index); });
    }
}

void ControlPanel::updateFocusButtons(int selectedIndex)
{
    focusLowButton.setToggleState(selectedIndex == 0, juce::dontSendNotification);
    focusMidButton.setToggleState(selectedIndex == 1, juce::dontSendNotification);
    focusHighButton.setToggleState(selectedIndex == 2, juce::dontSendNotification);
}

//==============================================================================
void ControlPanel::setupRotarySlider(juce::Slider& slider, int textBoxWidth, int textBoxHeight,
                                     const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, textBoxHeight);
    slider.setTextBoxIsEditable(true);

    if (suffix.isNotEmpty())
    {
        slider.setTextValueSuffix(suffix);
    }

    addAndMakeVisible(slider);
}

void ControlPanel::setupLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, GrainColours::kText);
    addAndMakeVisible(label);
}

//==============================================================================
void ControlPanel::setBypassAlpha(float alpha)
{
    grainSlider.setAlpha(alpha);
    warmthSlider.setAlpha(alpha);
    inputSlider.setAlpha(alpha);
    mixSlider.setAlpha(alpha);
    outputSlider.setAlpha(alpha);
    grainLabel.setAlpha(alpha);
    warmthLabel.setAlpha(alpha);
    inputLabel.setAlpha(alpha);
    mixLabel.setAlpha(alpha);
    outputLabel.setAlpha(alpha);
    focusLowButton.setAlpha(alpha);
    focusMidButton.setAlpha(alpha);
    focusHighButton.setAlpha(alpha);
    focusLabel.setAlpha(alpha);
}

//==============================================================================
void ControlPanel::paint(juce::Graphics& g)
{
    // Draw the dark rounded container behind the 3 focus buttons
    auto focusContainerBounds = focusLowButton.getBounds()
                                    .getUnion(focusMidButton.getBounds())
                                    .getUnion(focusHighButton.getBounds())
                                    .expanded(kFocusContainerPadding);

    g.setColour(GrainColours::kSurface);
    g.fillRoundedRectangle(focusContainerBounds.toFloat(), 8.0f);
}

//==============================================================================
void ControlPanel::resized()
{
    auto bounds = getLocalBounds();

    // Bottom row: INPUT | MIX | OUTPUT small knobs
    auto bottomRow = bounds.removeFromBottom(kBottomRowHeight);

    auto inputArea = bottomRow.removeFromLeft(kSmallKnobContainerWidth);
    auto outputArea = bottomRow.removeFromRight(kSmallKnobContainerWidth);
    auto mixArea = bottomRow;

    inputLabel.setBounds(inputArea.removeFromTop(18));
    inputSlider.setBounds(inputArea);

    mixLabel.setBounds(mixArea.removeFromTop(18));
    mixSlider.setBounds(mixArea);

    outputLabel.setBounds(outputArea.removeFromTop(18));
    outputSlider.setBounds(outputArea);

    // Gap between bottom row and main knob area
    bounds.removeFromBottom(12);

    // Center: GRAIN + WARM large knobs
    auto mainArea = bounds;
    mainArea.reduce(4, 0);

    // Reserve space for FOCUS selector
    constexpr int kFocusRowHeight = 50;
    auto focusRow = mainArea.removeFromBottom(kFocusRowHeight);

    // Large knob row
    auto knobRow = mainArea;
    const int knobWidth = (knobRow.getWidth() - 8) / 2;

    auto grainArea = knobRow.removeFromLeft(knobWidth);
    knobRow.removeFromLeft(8);
    auto warmthArea = knobRow;

    grainLabel.setBounds(grainArea.removeFromTop(20));
    grainSlider.setBounds(grainArea);

    warmthLabel.setBounds(warmthArea.removeFromTop(20));
    warmthSlider.setBounds(warmthArea);

    // Focus buttons (centered in focusRow)
    const int focusTotalWidth = (kFocusButtonWidth * 3) + (kFocusGap * 2);
    const int focusX = focusRow.getCentreX() - (focusTotalWidth / 2);
    const int focusY = focusRow.getY() + 4;

    focusLowButton.setBounds(focusX, focusY, kFocusButtonWidth, kFocusButtonHeight);
    focusMidButton.setBounds(focusX + kFocusButtonWidth + kFocusGap, focusY, kFocusButtonWidth, kFocusButtonHeight);
    focusHighButton.setBounds(focusX + ((kFocusButtonWidth + kFocusGap) * 2), focusY, kFocusButtonWidth,
                              kFocusButtonHeight);

    // Focus label below buttons
    focusLabel.setBounds(focusRow.getCentreX() - 30, focusY + kFocusButtonHeight + 2, 60, 16);
}
