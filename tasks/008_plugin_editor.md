# Task 008: Plugin Editor (Functional Layout)

## Objective

Create a functional plugin editor with organized controls, meters, and a basic visual style. This is **Phase A** — focused on usability and structure, not final visual polish.

```
┌────────────────────────────────────────────────┐
│              GRAIN                        [BP] │
│────────────────────────────────────────────────│
│                                                │
│   IN       [GRAIN]   [WARMTH]            OUT   │
│   ████      (big)     (big)              ████  │
│   ████                                   ████  │
│                                                │
│────────────────────────────────────────────────│
│  [INPUT]    [MIX]    [FOCUS]    [OUTPUT]       │
│  (small)   (small)  LOW MID HI  (small)        │
│                                                │
└────────────────────────────────────────────────┘
```

---

## Prerequisites

- Task 007 completed (full DSP chain with oversampling)

---

## Design Approach

### Phase A (This Task)
- Organized layout by functional sections
- All controls connected to APVTS parameters
- Working level meters (In/Out, L/R)
- Bypass toggle with visual feedback
- Consistent basic styling (not JUCE default grey)
- Fixed window size

### Phase B (Future Task — Custom UI)
- Visual design per React mockup specifications
- Custom knob graphics
- Refined color palette, typography
- Polished meters with segments
- **Optimize meter repaint** — restrict `repaint()` to meter areas only instead of full editor redraw (Phase A uses full `repaint()` at 30 FPS for simplicity)
- Reference: See design chat for specs

---

## Architecture

### Window Size

```cpp
// Fixed size for V1
constexpr int EDITOR_WIDTH = 500;
constexpr int EDITOR_HEIGHT = 350;
```

### Layout Sections

| Section | Contents | Position |
|---------|----------|----------|
| **Header** | Title "GRAIN", Bypass button | Top |
| **Main** | Grain knob, Warmth knob, Input/Output meters | Center |
| **Footer** | Input, Mix, Focus selector, Output | Bottom |

---

## Implementation

### Basic Color Palette (Phase A)

```cpp
namespace GrainColours
{
    const juce::Colour background { 0xff1c1917 };    // Dark stone
    const juce::Colour surface { 0xff292524 };       // Slightly lighter
    const juce::Colour text { 0xffa8a29e };          // Stone 400
    const juce::Colour textBright { 0xfff5f5f4 };    // Stone 100
    const juce::Colour accent { 0xffd97706 };        // Amber 600
    const juce::Colour accentDim { 0xff92400e };     // Amber 800
    const juce::Colour meterGreen { 0xff22c55e };    // Green 500
    const juce::Colour meterYellow { 0xffeab308 };   // Yellow 500
    const juce::Colour meterRed { 0xffef4444 };      // Red 500
}
```

### PluginEditor.h

> **Note:** Class name remains `GRAINAudioProcessorEditor` to match the existing Projucer-generated naming convention.

```cpp
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GRAINAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit GRAINAudioProcessorEditor(GRAINAudioProcessor&);
    ~GRAINAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawMeter(juce::Graphics&, juce::Rectangle<float> area,
                   float levelL, float levelR, const juce::String& label);

    GRAINAudioProcessor& processor;

    // Main controls (creative)
    juce::Slider grainSlider;
    juce::Slider warmthSlider;

    // Secondary controls (utility)
    juce::Slider inputSlider;
    juce::Slider mixSlider;
    juce::ComboBox focusSelector;
    juce::Slider outputSlider;

    // Bypass
    juce::TextButton bypassButton { "BYPASS" };

    // Labels
    juce::Label grainLabel { {}, "GRAIN" };
    juce::Label warmthLabel { {}, "WARMTH" };
    juce::Label inputLabel { {}, "INPUT" };
    juce::Label mixLabel { {}, "MIX" };
    juce::Label focusLabel { {}, "FOCUS" };
    juce::Label outputLabel { {}, "OUTPUT" };

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> focusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // Meter display values (smoothed)
    float displayInputL = 0.0f, displayInputR = 0.0f;
    float displayOutputL = 0.0f, displayOutputR = 0.0f;

    static constexpr float METER_DECAY = 0.85f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRAINAudioProcessorEditor)
};
```

### PluginEditor.cpp

```cpp
#include "PluginEditor.h"

namespace
{
    constexpr int EDITOR_WIDTH = 500;
    constexpr int EDITOR_HEIGHT = 350;
}

GRAINAudioProcessorEditor::GRAINAudioProcessorEditor(GRAINAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(EDITOR_WIDTH, EDITOR_HEIGHT);

    // === Grain (main — creative) ===
    // Visual label: "GRAIN" — Parameter ID: "drive"
    grainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    grainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(grainSlider);
    grainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "drive", grainSlider);

    grainLabel.setJustificationType(juce::Justification::centred);
    grainLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(grainLabel);

    // === Warmth (main — creative) ===
    warmthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    warmthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(warmthSlider);
    warmthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "warmth", warmthSlider);

    warmthLabel.setJustificationType(juce::Justification::centred);
    warmthLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(warmthLabel);

    // === Input (secondary — utility) ===
    inputSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    inputSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    inputSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(inputSlider);
    inputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "inputGain", inputSlider);

    inputLabel.setJustificationType(juce::Justification::centred);
    inputLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(inputLabel);

    // === Mix (secondary — utility) ===
    mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    addAndMakeVisible(mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "mix", mixSlider);

    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(mixLabel);

    // === Focus (secondary — selector) ===
    // Items in uppercase for UI display; attachment maps by index to parameter values
    focusSelector.addItem("LOW", 1);
    focusSelector.addItem("MID", 2);
    focusSelector.addItem("HIGH", 3);
    addAndMakeVisible(focusSelector);
    focusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "focus", focusSelector);

    focusLabel.setJustificationType(juce::Justification::centred);
    focusLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(focusLabel);

    // === Output ===
    outputSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    outputSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(outputSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "output", outputSlider);

    outputLabel.setJustificationType(juce::Justification::centred);
    outputLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(outputLabel);

    // === Bypass ===
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, GrainColours::surface);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, GrainColours::accent);
    bypassButton.setColour(juce::TextButton::textColourOffId, GrainColours::text);
    bypassButton.setColour(juce::TextButton::textColourOnId, GrainColours::textBright);
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getAPVTS(), "bypass", bypassButton);

    // Start meter timer
    startTimerHz(30);
}

GRAINAudioProcessorEditor::~GRAINAudioProcessorEditor()
{
    stopTimer();
}

void GRAINAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(GrainColours::background);

    // Header area
    auto headerArea = getLocalBounds().removeFromTop(50);
    g.setColour(GrainColours::surface);
    g.fillRect(headerArea);

    // Title — JUCE 8 Font API
    g.setColour(GrainColours::textBright);
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    g.drawText("GRAIN", headerArea.reduced(15, 0), juce::Justification::centredLeft);

    // Meters
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50);  // Header
    bounds.removeFromBottom(100);  // Footer

    auto meterWidth = 40;
    auto inputMeterArea = bounds.removeFromLeft(meterWidth + 20).toFloat().reduced(10, 20);
    auto outputMeterArea = bounds.removeFromRight(meterWidth + 20).toFloat().reduced(10, 20);

    drawMeter(g, inputMeterArea, displayInputL, displayInputR, "IN");
    drawMeter(g, outputMeterArea, displayOutputL, displayOutputR, "OUT");

    // Footer separator
    g.setColour(GrainColours::surface);
    g.fillRect(getLocalBounds().removeFromBottom(100));
}

void GRAINAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> area,
                                           float levelL, float levelR, const juce::String& label)
{
    auto labelHeight = 20.0f;
    auto labelArea = area.removeFromTop(labelHeight);

    g.setColour(GrainColours::text);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(label, labelArea, juce::Justification::centred);

    auto meterArea = area.reduced(2, 0);
    auto meterBarWidth = meterArea.getWidth() / 2.0f - 2.0f;

    auto leftArea = meterArea.removeFromLeft(meterBarWidth);
    meterArea.removeFromLeft(4);  // Gap
    auto rightArea = meterArea;

    // Draw meter backgrounds
    g.setColour(juce::Colours::black);
    g.fillRect(leftArea);
    g.fillRect(rightArea);

    // Draw levels
    auto drawLevel = [&](juce::Rectangle<float> meterRect, float level)
    {
        float dbLevel = juce::Decibels::gainToDecibels(level, -60.0f);
        float normalized = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        auto levelRect = meterRect;
        auto levelHeight = levelRect.getHeight() * normalized;
        levelRect = levelRect.removeFromBottom(levelHeight);

        // Gradient: green -> yellow -> red
        if (normalized < 0.7f)
            g.setColour(GrainColours::meterGreen);
        else if (normalized < 0.9f)
            g.setColour(GrainColours::meterYellow);
        else
            g.setColour(GrainColours::meterRed);

        g.fillRect(levelRect);
    };

    drawLevel(leftArea.reduced(1), levelL);
    drawLevel(rightArea.reduced(1), levelR);
}

void GRAINAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header
    auto headerArea = bounds.removeFromTop(50);
    bypassButton.setBounds(headerArea.removeFromRight(80).reduced(10));

    // Footer (secondary controls — 4 items)
    auto footerArea = bounds.removeFromBottom(100);
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

    // Main area (big knobs — creative controls)
    auto mainArea = bounds;
    mainArea.reduce(70, 20);  // Leave space for meters

    auto knobWidth = mainArea.getWidth() / 2;

    auto grainArea = mainArea.removeFromLeft(knobWidth);
    grainLabel.setBounds(grainArea.removeFromTop(25));
    grainSlider.setBounds(grainArea);

    auto warmthArea = mainArea;
    warmthLabel.setBounds(warmthArea.removeFromTop(25));
    warmthSlider.setBounds(warmthArea);
}

void GRAINAudioProcessorEditor::timerCallback()
{
    // Smooth meter decay
    // NOTE: Phase A repaints entire editor at 30 FPS for simplicity.
    // Phase B should optimize to repaint only meter areas.
    float inL = processor.inputLevelL.load();
    float inR = processor.inputLevelR.load();
    float outL = processor.outputLevelL.load();
    float outR = processor.outputLevelR.load();

    displayInputL = std::max(inL, displayInputL * METER_DECAY);
    displayInputR = std::max(inR, displayInputR * METER_DECAY);
    displayOutputL = std::max(outL, displayOutputL * METER_DECAY);
    displayOutputR = std::max(outR, displayOutputR * METER_DECAY);

    repaint();
}
```

### Required: Level Measurement in Processor

Ensure `PluginProcessor.h` has (public section):

```cpp
// Atomic level values for thread-safe GUI reads
std::atomic<float> inputLevelL { 0.0f };
std::atomic<float> inputLevelR { 0.0f };
std::atomic<float> outputLevelL { 0.0f };
std::atomic<float> outputLevelR { 0.0f };
```

And `processBlock()` measures levels:

```cpp
// At start of processBlock (before processing)
inputLevelL.store(buffer.getMagnitude(0, 0, buffer.getNumSamples()));
if (buffer.getNumChannels() > 1)
    inputLevelR.store(buffer.getMagnitude(1, 0, buffer.getNumSamples()));

// At end of processBlock (after processing)
outputLevelL.store(buffer.getMagnitude(0, 0, buffer.getNumSamples()));
if (buffer.getNumChannels() > 1)
    outputLevelR.store(buffer.getMagnitude(1, 0, buffer.getNumSamples()));
```

### Required: APVTS Encapsulation

Move `apvts` to private and add accessor:

```cpp
// In PluginProcessor.h — public section
juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

// In PluginProcessor.h — private section (move from public)
juce::AudioProcessorValueTreeState apvts;
```

---

## Parameter ID Mapping

| Control | Visual Label | Parameter ID | Type | Section |
|---------|-------------|--------------|------|---------|
| Grain knob | "GRAIN" | `drive` | float 0-1 | Main (big) |
| Warmth knob | "WARMTH" | `warmth` | float 0-1 | Main (big) |
| Input knob | "INPUT" | `inputGain` | float -12 to +12 | Footer (small) |
| Mix knob | "MIX" | `mix` | float 0-1 | Footer (small) |
| Focus selector | "FOCUS" (items: LOW, MID, HIGH) | `focus` | choice 0-2 | Footer (small) |
| Output knob | "OUTPUT" | `output` | float -12 to +12 | Footer (small) |
| Bypass button | "BYPASS" | `bypass` | bool | Header |

> **Important:** The visual label "GRAIN" maps to parameter ID `"drive"` — the knob name is a user-facing alias.
> Focus ComboBox displays items in uppercase ("LOW", "MID", "HIGH") while the parameter defines ("Low", "Mid", "High"). The attachment maps by index, so this is safe.
> Input Gain allows the user to adjust the signal level entering the saturation stages without changing the channel volume in the DAW.

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Source/PluginEditor.h` | Rewrite with new layout |
| `Source/PluginEditor.cpp` | Implement editor |
| `Source/PluginProcessor.h` | Add level atomics, add `getAPVTS()`, move `apvts` to private |
| `Source/PluginProcessor.cpp` | Add level measurement in `processBlock()` |

---

## Acceptance Criteria

### Layout
- [ ] Fixed window size (500 x 350)
- [ ] Header with title and bypass button
- [ ] Main section with Grain and Warmth knobs (larger)
- [ ] Footer with Input, Mix, Focus, Output (smaller)
- [ ] Input and Output meters on sides

### Functionality
- [ ] All controls connected to correct parameters
- [ ] Grain knob connected to `"drive"` parameter ID
- [ ] Warmth knob connected to `"warmth"` parameter ID
- [ ] Input knob connected to `"inputGain"` parameter ID
- [ ] Bypass button toggles with visual feedback
- [ ] Focus selector shows LOW/MID/HIGH (uppercase)
- [ ] Meters respond to audio levels
- [ ] Meters decay smoothly

### Visual
- [ ] Dark background (not JUCE default grey)
- [ ] Consistent color palette
- [ ] Labels clearly visible
- [ ] Bypass state obvious (color change)

### Quality
- [ ] No crashes on open/close
- [ ] Responsive to parameter changes from host
- [ ] Meters don't cause CPU spikes
- [ ] pluginval still passes
- [ ] No deprecation warnings from JUCE 8 Font API

---

## Testing Checklist

```bash
# 1. Build
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 2. Run pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 3. Test in DAW
# - Load plugin
# - Verify all controls visible
# - Adjust each parameter
# - Automate parameters from host
# - Toggle bypass
# - Verify meters respond to audio
```

---

## Technical Notes

### Why Timer for Meters?

Using `startTimerHz(30)` decouples meter display from audio thread:
- Audio thread: writes levels to atomic (fast, non-blocking)
- GUI thread: reads atomics and repaints (30 FPS)

This avoids blocking the audio thread with repaint calls.

### APVTS Attachments

Attachments automatically:
- Sync control <-> parameter
- Handle host automation
- Manage undo/redo

Always use attachments rather than manual callbacks.

### Meter Decay Math

```
displayLevel = max(newLevel, displayLevel * METER_DECAY)
```

At 30 FPS with decay 0.85:
- Peak holds briefly
- Falls to 50% in ~130ms
- Smooth visual without flicker

### JUCE 8 Font API

This task uses the modern JUCE 8 `FontOptions` API to avoid deprecation warnings:
```cpp
// JUCE 8 (correct)
juce::Font(juce::FontOptions(24.0f).withStyle("Bold"))

// Legacy (deprecated in JUCE 8)
// juce::Font(24.0f, juce::Font::bold)
```

---

## Deferred to Phase B

The future custom UI task will:
- Replace standard sliders with custom knob graphics
- Add segmented meter display
- Implement the full visual design from the React mockup
- Add LED-style bypass indicator
- Refine typography and spacing
- **Optimize meter repaint** — use `repaint(meterBounds)` instead of full `repaint()` to reduce GPU/CPU load

This task (Phase A) establishes the **structure and connectivity**. Phase B adds **visual polish and optimization**.
