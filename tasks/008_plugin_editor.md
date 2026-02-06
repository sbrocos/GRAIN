# Task 008: Plugin Editor (Functional Layout)

## Objective

Create a functional plugin editor with organized controls, meters, and a basic visual style. This is **Phase A** — focused on usability and structure, not final visual polish.

```
┌────────────────────────────────────────────┐
│              GRAIN                    [BP] │
│────────────────────────────────────────────│
│                                            │
│   IN        [GRAIN]    [MIX]         OUT   │
│   ████       (big)     (big)         ████  │
│   ████                               ████  │
│                                            │
│────────────────────────────────────────────│
│  [WARMTH]    [FOCUS]    [OUTPUT]           │
│   (small)   LOW MID HI   (small)           │
│                                            │
└────────────────────────────────────────────┘
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
| **Main** | Grain knob, Mix knob, Input/Output meters | Center |
| **Footer** | Warmth, Focus selector, Output | Bottom |

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

```cpp
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GrainEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit GrainEditor(GrainProcessor&);
    ~GrainEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawMeter(juce::Graphics&, juce::Rectangle<float> area, 
                   float levelL, float levelR, const juce::String& label);

    GrainProcessor& processor;

    // Main controls
    juce::Slider grainSlider;
    juce::Slider mixSlider;
    
    // Secondary controls
    juce::Slider warmthSlider;
    juce::ComboBox focusSelector;
    juce::Slider outputSlider;
    
    // Bypass
    juce::TextButton bypassButton { "BYPASS" };
    
    // Labels
    juce::Label grainLabel { {}, "GRAIN" };
    juce::Label mixLabel { {}, "MIX" };
    juce::Label warmthLabel { {}, "WARMTH" };
    juce::Label focusLabel { {}, "FOCUS" };
    juce::Label outputLabel { {}, "OUTPUT" };
    
    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> focusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Meter display values (smoothed)
    float displayInputL = 0.0f, displayInputR = 0.0f;
    float displayOutputL = 0.0f, displayOutputR = 0.0f;
    
    static constexpr float METER_DECAY = 0.85f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainEditor)
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

GrainEditor::GrainEditor(GrainProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(EDITOR_WIDTH, EDITOR_HEIGHT);
    
    // === Grain (main) ===
    grainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    grainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(grainSlider);
    grainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "grain", grainSlider);
    
    grainLabel.setJustificationType(juce::Justification::centred);
    grainLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(grainLabel);
    
    // === Mix (main) ===
    mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "mix", mixSlider);
    
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(mixLabel);
    
    // === Warmth ===
    warmthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    warmthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    addAndMakeVisible(warmthSlider);
    warmthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "warmth", warmthSlider);
    
    warmthLabel.setJustificationType(juce::Justification::centred);
    warmthLabel.setColour(juce::Label::textColourId, GrainColours::text);
    addAndMakeVisible(warmthLabel);
    
    // === Focus (selector) ===
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

GrainEditor::~GrainEditor()
{
    stopTimer();
}

void GrainEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(GrainColours::background);
    
    // Header area
    auto headerArea = getLocalBounds().removeFromTop(50);
    g.setColour(GrainColours::surface);
    g.fillRect(headerArea);
    
    // Title
    g.setColour(GrainColours::textBright);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
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

void GrainEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> area, 
                            float levelL, float levelR, const juce::String& label)
{
    auto labelHeight = 20.0f;
    auto labelArea = area.removeFromTop(labelHeight);
    
    g.setColour(GrainColours::text);
    g.setFont(12.0f);
    g.drawText(label, labelArea, juce::Justification::centred);
    
    auto meterArea = area.reduced(2, 0);
    auto meterWidth = meterArea.getWidth() / 2.0f - 2.0f;
    
    auto leftArea = meterArea.removeFromLeft(meterWidth);
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

void GrainEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Header
    auto headerArea = bounds.removeFromTop(50);
    bypassButton.setBounds(headerArea.removeFromRight(80).reduced(10));
    
    // Footer (secondary controls)
    auto footerArea = bounds.removeFromBottom(100);
    auto footerControls = footerArea.reduced(20, 10);
    
    auto controlWidth = footerControls.getWidth() / 3;
    
    auto warmthArea = footerControls.removeFromLeft(controlWidth);
    warmthLabel.setBounds(warmthArea.removeFromTop(20));
    warmthSlider.setBounds(warmthArea);
    
    auto focusArea = footerControls.removeFromLeft(controlWidth);
    focusLabel.setBounds(focusArea.removeFromTop(20));
    focusSelector.setBounds(focusArea.reduced(10, 20));
    
    auto outputArea = footerControls;
    outputLabel.setBounds(outputArea.removeFromTop(20));
    outputSlider.setBounds(outputArea);
    
    // Main area (big knobs)
    auto mainArea = bounds;
    mainArea.reduce(70, 20);  // Leave space for meters
    
    auto knobWidth = mainArea.getWidth() / 2;
    
    auto grainArea = mainArea.removeFromLeft(knobWidth);
    grainLabel.setBounds(grainArea.removeFromTop(25));
    grainSlider.setBounds(grainArea);
    
    auto mixArea = mainArea;
    mixLabel.setBounds(mixArea.removeFromTop(25));
    mixSlider.setBounds(mixArea);
}

void GrainEditor::timerCallback()
{
    // Smooth meter decay
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

Ensure `PluginProcessor.h` has:

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

### Required: APVTS Accessor

```cpp
// In PluginProcessor.h
juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
```

---

## Parameter ID Mapping

| Control | Parameter ID | Type |
|---------|--------------|------|
| Grain knob | `grain` | float 0-1 |
| Mix knob | `mix` | float 0-1 |
| Warmth knob | `warmth` | float 0-1 |
| Focus selector | `focus` | choice 0-2 |
| Output knob | `output` | float -12 to +12 |
| Bypass button | `bypass` | float 0-1 (as bool) |

**Note:** The parameter IDs must match what was defined in Task 002 and subsequent tasks. If IDs differ, update either the parameters or the attachments.

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Source/PluginEditor.h` | Rewrite with new layout |
| `Source/PluginEditor.cpp` | Implement editor |
| `Source/PluginProcessor.h` | Add level atomics, getAPVTS() |
| `Source/PluginProcessor.cpp` | Add level measurement |

---

## Acceptance Criteria

### Layout
- [ ] Fixed window size (500 x 350)
- [ ] Header with title and bypass button
- [ ] Main section with Grain and Mix knobs (larger)
- [ ] Footer with Warmth, Focus, Output (smaller)
- [ ] Input and Output meters on sides

### Functionality
- [ ] All controls connected to correct parameters
- [ ] Bypass button toggles with visual feedback
- [ ] Focus selector shows Low/Mid/High
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

---

## Testing Checklist

```bash
# 1. Build
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 2. Run pluginval
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

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
- Sync control ↔ parameter
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

---

## Phase B Preview (Future Task)

The future custom UI task will:
- Replace standard sliders with custom knob graphics
- Add segmented meter display
- Implement the full visual design from the React mockup
- Add LED-style bypass indicator
- Refine typography and spacing

This task (Phase A) establishes the **structure and connectivity**. Phase B adds **visual polish**.
