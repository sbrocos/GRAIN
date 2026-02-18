# Task 010: Custom UI (Phase B) — Revised

## Objective

Replace the functional Phase A editor with a polished custom UI based on the approved JSX mockup. Phase A established structure and connectivity. Phase B adds visual refinement: custom dot-style knobs, vertical segmented meters with peak hold, LED bypass indicator, editable value fields, and consistent typography.

**Reference:** `GrainUI.jsx` (React mockup)

**Constraint:** Visual-only transformation. Same parameters, same DSP.

---

## Design Summary (from JSX Mockup)

### Layout Structure

```
┌─────────────────────────────────────────────────────────────┐
│  GRAIN                                              ⏻       │
│  MICRO-HARMONIC SATURATION                                  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ▐█▌         GRAIN        WARM          ▐█▌               │
│   IN          (large)     (large)        OUT               │
│   ▐█▌           ◉           ◉            ▐█▌               │
│                                                             │
│                [ LOW | MID | HIGH ]                         │
│                      FOCUS                                  │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   INPUT              MIX              OUTPUT                │
│   (small)          (medium)          (small)                │
│     ◉                 ◉                 ◉                   │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  v1.0.0                          Sergio Brocos © 2025       │
└─────────────────────────────────────────────────────────────┘
```

### Window Dimensions

| Target | Width | Height |
|--------|-------|--------|
| VST3 Plugin | 500 px | 420 px |
| Standalone | 500 px | 590 px |

---

## Prerequisites

- Task 008 completed (Phase A — functional layout)
- Task 009b completed (Standalone file player)
- Approved JSX mockup (`GrainUI.jsx`)

---

## Color Palette

Based on JSX mockup. Update `Source/GrainColours.h`:

```cpp
namespace GrainColours
{
    // === Background & Surfaces ===
    inline const juce::Colour kBackground   {0xffB6C1B9};  // Sage green (main bg)
    inline const juce::Colour kSurface      {0xff1a1a1a};  // Knob body, meters bg
    inline const juce::Colour kSurfaceLight {0xff3a3a3a};  // Knob ring, inactive dots
    
    // === Accent ===
    inline const juce::Colour kAccent       {0xffd97706};  // Active dots, selected buttons
    
    // === Text ===
    inline const juce::Colour kText         {0xff1a1a1a};  // Labels on light bg
    inline const juce::Colour kTextDim      {0xff4a4a4a};  // Subtitle, footer
    inline const juce::Colour kTextBright   {0xffffffff};  // Text on dark bg
    inline const juce::Colour kTextMuted    {0xff888888};  // Inactive button text
    
    // === Meters ===
    inline const juce::Colour kMeterGreen   {0xff22c55e};  // 0-75%
    inline const juce::Colour kMeterYellow  {0xffeab308};  // 75-90%
    inline const juce::Colour kMeterRed     {0xffef4444};  // 90-100%
    inline const juce::Colour kMeterOff     {0xff3a3a3a};  // Inactive segments
    
    // === Bypass LED ===
    inline const juce::Colour kBypassOn     {0xffd97706};  // Processing (orange)
    inline const juce::Colour kBypassOff    {0xff1a1a1a};  // Bypassed (dark)
}
```

---

## Subtasks

### Subtask 1: GrainLookAndFeel + Dot-Style Knobs

**Objective:** Create custom `LookAndFeel` with dot-style rotary sliders matching the JSX mockup.

**Files to create:**
- `Source/UI/GrainLookAndFeel.h`
- `Source/UI/GrainLookAndFeel.cpp`

**Files to modify:**
- `Source/PluginEditor.h`
- `Source/PluginEditor.cpp`
- `Source/GrainColours.h`
- `GRAIN.jucer`

**Knob Design Spec (from JSX):**

| Element | Description |
|---------|-------------|
| Dot arc | Circle of dots around knob, 270° range, opening at bottom |
| Active dots | Orange (#d97706), from min to current value |
| Inactive dots | Dark gray (#1a1a1a) |
| Knob body | Black circle with subtle gradient (#2a2a2a → #1a1a1a) |
| Inner ring | 2px border (#3a3a3a), inset 3px |
| Indicator | White dot on knob face, rotates with value |

**Knob Sizes:**

| Size | Diameter | Dot count | Dot radius | Usage |
|------|----------|-----------|------------|-------|
| Large | 120 px | 35 | 3 px | GRAIN, WARM |
| Medium | 80 px | 24 | 3.5 px | MIX |
| Small | 60 px | 21 | 3 px | INPUT, OUTPUT |

**Implementation:**

```cpp
void GrainLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider) override
{
    // Determine size category from slider properties or dimensions
    const bool isLarge = width >= 140;
    const bool isMedium = width >= 100 && width < 140;
    
    const int dotCount = isLarge ? 35 : (isMedium ? 24 : 21);
    const float dotRadius = isLarge ? 3.0f : (isMedium ? 3.5f : 3.0f);
    const float knobRadius = isLarge ? 60.0f : (isMedium ? 40.0f : 30.0f);
    
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto centre = bounds.getCentre();
    
    // 1. Draw dot arc (opening at bottom)
    const float dotArcRadius = knobRadius + 15.0f;
    for (int i = 0; i < dotCount; ++i)
    {
        float proportion = static_cast<float>(i) / static_cast<float>(dotCount - 1);
        float angle = rotaryStartAngle + proportion * (rotaryEndAngle - rotaryStartAngle);
        
        float dotX = centre.x + dotArcRadius * std::cos(angle - juce::MathConstants<float>::halfPi);
        float dotY = centre.y + dotArcRadius * std::sin(angle - juce::MathConstants<float>::halfPi);
        
        bool isActive = proportion <= sliderPos;
        g.setColour(isActive ? GrainColours::kAccent : GrainColours::kSurface);
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }
    
    // 2. Draw knob body with gradient
    juce::ColourGradient gradient(
        juce::Colour(0xff2a2a2a), centre.x - knobRadius * 0.5f, centre.y - knobRadius * 0.5f,
        juce::Colour(0xff1a1a1a), centre.x + knobRadius * 0.5f, centre.y + knobRadius * 0.5f,
        false);
    g.setGradientFill(gradient);
    g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    
    // 3. Draw inner ring
    g.setColour(GrainColours::kSurfaceLight);
    const float ringInset = 3.0f;
    g.drawEllipse(centre.x - knobRadius + ringInset, centre.y - knobRadius + ringInset,
                  (knobRadius - ringInset) * 2.0f, (knobRadius - ringInset) * 2.0f, 2.0f);
    
    // 4. Draw indicator dot
    float indicatorAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    float indicatorDistance = knobRadius - (isLarge ? 22.0f : (isMedium ? 16.0f : 14.0f));
    float indX = centre.x + indicatorDistance * std::cos(indicatorAngle - juce::MathConstants<float>::halfPi);
    float indY = centre.y + indicatorDistance * std::sin(indicatorAngle - juce::MathConstants<float>::halfPi);
    
    float indicatorRadius = isLarge ? 5.5f : (isMedium ? 4.0f : 3.5f);
    g.setColour(juce::Colours::white);
    g.fillEllipse(indX - indicatorRadius, indY - indicatorRadius, indicatorRadius * 2.0f, indicatorRadius * 2.0f);
}
```

**Acceptance criteria:**
- [ ] All 5 knobs render with dot-style arc
- [ ] Large knobs (GRAIN, WARM) visibly bigger than small (INPUT, OUTPUT)
- [ ] MIX knob is medium size
- [ ] Active dots are orange, inactive are dark
- [ ] White indicator dot rotates correctly
- [ ] Dot arc opening is at bottom (standard knob orientation)
- [ ] pluginval passes
- [ ] All unit tests pass

---

### Subtask 2: Editable Value Fields

**Objective:** Make the value display below each knob editable via double-click.

**Files to modify:**
- `Source/PluginEditor.cpp`
- `Source/UI/GrainLookAndFeel.cpp`

**Behavior Spec:**

| Action | Result |
|--------|--------|
| Normal view | Appears as static text ("25%", "0 dB") |
| Double-click on value | Activates text editing, selects all text |
| Type value | Replaces current value |
| Enter / click outside | Confirms and applies value |
| Escape | Cancels edit, restores previous value |
| Out-of-range value | Clamps to valid range |

**Implementation:**

```cpp
// In PluginEditor constructor - configure each slider
grainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
grainSlider.setTextBoxIsEditable(true);

// In GrainLookAndFeel - style the text editor to look like plain text
void GrainLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                                 juce::TextEditor& editor) override
{
    // Transparent background - appears as plain text
    g.setColour(juce::Colours::transparentBlack);
    g.fillAll();
}

void GrainLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                              juce::TextEditor& editor) override
{
    // Only show subtle outline when focused (editing)
    if (editor.hasKeyboardFocus(true))
    {
        g.setColour(GrainColours::kAccent.withAlpha(0.5f));
        g.drawRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), 
                               static_cast<float>(height), 2.0f, 1.0f);
    }
}

// Optional: Style the text
juce::Font GrainLookAndFeel::getLabelFont(juce::Label& label) override
{
    return juce::Font(juce::FontOptions().withHeight(14.0f));
}
```

**Acceptance criteria:**
- [ ] Value fields appear as static text by default
- [ ] Double-click activates edit mode
- [ ] Text is selected on edit activation
- [ ] Enter confirms value
- [ ] Escape cancels edit
- [ ] Invalid values are clamped
- [ ] Subtle outline appears only during editing

---

### Subtask 3: Vertical Segmented Meters with Peak Hold

**Objective:** Replace placeholder meters with vertical LED-style segmented meters including peak hold.

**Files to modify:**
- `Source/PluginEditor.h`
- `Source/PluginEditor.cpp`

**Meter Design Spec (from JSX):**

| Element | Value |
|---------|-------|
| Segments per channel | 32 |
| Segment width | 8 px |
| Segment height | 7 px |
| Gap between segments | ~2 px |
| Gap between L/R | 4 px |
| Background | #1a1a1a rounded rect |
| Padding | 8 px |

**Color Zones:**

| Zone | Segment range | Color |
|------|---------------|-------|
| Green | 0 - 75% (segments 0-23) | #22c55e |
| Yellow | 75 - 90% (segments 24-28) | #eab308 |
| Red | 90 - 100% (segments 29-31) | #ef4444 |
| Off | — | #3a3a3a |

**Peak Hold Spec:**

| Phase | Duration | Behavior |
|-------|----------|----------|
| Capture | Instant | When new peak exceeds current, update immediately |
| Hold | ~1 second | Peak segment stays illuminated (30 frames at 30 FPS) |
| Decay | Gradual | Multiply by 0.95 each frame after hold expires |

**Implementation:**

```cpp
// In PluginEditor.h
struct PeakHold
{
    float peakLevel = 0.0f;
    int holdCounter = 0;
    
    void update(float newLevel)
    {
        if (newLevel >= peakLevel)
        {
            peakLevel = newLevel;
            holdCounter = 30;  // ~1s at 30 FPS
        }
        else if (holdCounter > 0)
        {
            --holdCounter;
        }
        else
        {
            peakLevel *= 0.95f;  // Gradual decay
        }
    }
    
    void reset()
    {
        peakLevel = 0.0f;
        holdCounter = 0;
    }
};

// Members
PeakHold peakHoldInL, peakHoldInR;
PeakHold peakHoldOutL, peakHoldOutR;
juce::Rectangle<int> inputMeterBounds;
juce::Rectangle<int> outputMeterBounds;

// In PluginEditor.cpp
void GrainAudioProcessorEditor::timerCallback()
{
    // Get current levels from processor
    float inL = audioProcessor.getInputLevelL();
    float inR = audioProcessor.getInputLevelR();
    float outL = audioProcessor.getOutputLevelL();
    float outR = audioProcessor.getOutputLevelR();
    
    // Update peak hold
    peakHoldInL.update(inL);
    peakHoldInR.update(inR);
    peakHoldOutL.update(outL);
    peakHoldOutR.update(outR);
    
    // Only repaint meter areas (optimization)
    repaint(inputMeterBounds);
    repaint(outputMeterBounds);
}

void GrainAudioProcessorEditor::drawMeter(juce::Graphics& g,
                                           juce::Rectangle<int> bounds,
                                           float levelL, float levelR,
                                           float peakL, float peakR)
{
    const int numSegments = 32;
    const int segmentWidth = 8;
    const int segmentHeight = 7;
    const int segmentGap = 2;
    const int channelGap = 4;
    const int padding = 8;
    
    // Draw background
    g.setColour(GrainColours::kSurface);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    // Calculate positions
    int totalChannelWidth = segmentWidth * 2 + channelGap;
    int startX = bounds.getCentreX() - totalChannelWidth / 2;
    
    auto drawChannel = [&](int x, float level, float peak)
    {
        int litSegments = static_cast<int>(level * numSegments);
        int peakSegment = juce::jmax(0, static_cast<int>(peak * numSegments) - 1);
        
        for (int i = 0; i < numSegments; ++i)
        {
            // Bottom to top
            int y = bounds.getBottom() - padding - (i + 1) * (segmentHeight + segmentGap);
            bool isLit = i < litSegments;
            bool isPeak = (i == peakSegment) && (peak > level) && (peak > 0.01f);
            
            juce::Colour colour;
            float position = static_cast<float>(i) / numSegments;
            
            if (!isLit && !isPeak)
            {
                colour = GrainColours::kMeterOff;
            }
            else if (position > 0.9f)
            {
                colour = GrainColours::kMeterRed;
            }
            else if (position > 0.75f)
            {
                colour = GrainColours::kMeterYellow;
            }
            else
            {
                colour = GrainColours::kMeterGreen;
            }
            
            // Peak hold with reduced alpha to differentiate
            if (isPeak && !isLit)
                colour = colour.withAlpha(0.6f);
            
            g.setColour(colour);
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                                   static_cast<float>(segmentWidth), 
                                   static_cast<float>(segmentHeight), 1.0f);
        }
    };
    
    drawChannel(startX, levelL, peakL);
    drawChannel(startX + segmentWidth + channelGap, levelR, peakR);
}
```

**Acceptance criteria:**
- [ ] Meters display vertically (bottom to top)
- [ ] 32 segments per channel, L and R visible
- [ ] Green/yellow/red color zones correct
- [ ] Inactive segments visible (dark gray)
- [ ] Peak hold segment stays lit ~1 second
- [ ] Peak decays gradually after hold
- [ ] Only meter bounds repainted each frame (optimization)
- [ ] Responds correctly to audio levels

---

### Subtask 4: Bypass Button (LED Style)

**Objective:** Style bypass as circular LED button per JSX mockup.

**Files to modify:**
- `Source/PluginEditor.cpp`
- `Source/UI/GrainLookAndFeel.cpp`

**Design Spec:**

| State | Background | Icon color | Shadow |
|-------|------------|------------|--------|
| Processing (bypass=false) | #d97706 | white | glow |
| Bypassed (bypass=true) | #1a1a1a | #666666 | inset |

**Size:** 40 × 40 px circular

**Implementation:**

```cpp
void GrainLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                             juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool isMouseOver, bool isButtonDown) override
{
    auto bounds = button.getLocalBounds().toFloat();
    
    if (button.getName() == "bypass")
    {
        bool isOn = button.getToggleState();  // true = bypassed
        
        // Background
        g.setColour(isOn ? GrainColours::kBypassOff : GrainColours::kBypassOn);
        g.fillEllipse(bounds);
        
        // Glow when processing
        if (!isOn)
        {
            g.setColour(GrainColours::kAccent.withAlpha(0.3f));
            g.drawEllipse(bounds.expanded(2.0f), 4.0f);
        }
        
        return;
    }
    
    // Default button rendering...
}
```

**Acceptance criteria:**
- [ ] Bypass button is circular
- [ ] Orange when processing, dark when bypassed
- [ ] Power icon visible and correctly colored
- [ ] Click toggles state
- [ ] UI opacity reduces to 40% when bypassed

---

### Subtask 5: Focus Selector (3-way Switch)

**Objective:** Implement Focus as 3 grouped buttons (Opción A).

**Design Spec:**

```
┌─────────────────────────────────┐
│  LOW  │  MID  │  HIGH │        │
└─────────────────────────────────┘
```

| Element | Value |
|---------|-------|
| Container bg | #1a1a1a |
| Container padding | 3 px |
| Button padding | 5px vertical, 14px horizontal |
| Active bg | #d97706 |
| Active text | #ffffff |
| Inactive bg | transparent |
| Inactive text | #888888 |
| Border radius | 4 px (buttons), 8 px (container) |

**Implementation (3 TextButtons with radio behavior):**

```cpp
// In PluginEditor.h
juce::TextButton focusLowButton{"LOW"};
juce::TextButton focusMidButton{"MID"};
juce::TextButton focusHighButton{"HIGH"};

// In constructor
auto updateFocusButtons = [this](int selected)
{
    focusLowButton.setToggleState(selected == 0, juce::dontSendNotification);
    focusMidButton.setToggleState(selected == 1, juce::dontSendNotification);
    focusHighButton.setToggleState(selected == 2, juce::dontSendNotification);
};

focusLowButton.onClick = [this, updateFocusButtons] {
    // Assuming focus parameter normalized 0-1 maps to 0,1,2
    focusParam->setValueNotifyingHost(0.0f);
    updateFocusButtons(0);
};
focusMidButton.onClick = [this, updateFocusButtons] {
    focusParam->setValueNotifyingHost(0.5f);
    updateFocusButtons(1);
};
focusHighButton.onClick = [this, updateFocusButtons] {
    focusParam->setValueNotifyingHost(1.0f);
    updateFocusButtons(2);
};
```

**Acceptance criteria:**
- [ ] Three buttons in horizontal row
- [ ] Only one can be selected at a time
- [ ] Selected button is orange with white text
- [ ] Unselected buttons have gray text on dark bg
- [ ] Connected to Focus parameter via APVTS
- [ ] Syncs when parameter changes from DAW automation

---

### Subtask 6: Layout & Polish

**Objective:** Implement exact layout from JSX mockup and final polish.

**Layout Implementation:**

```cpp
void GrainAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Header: 60px
    auto headerArea = bounds.removeFromTop(60);
    // Title left, bypass right
    
    // Main area
    auto mainArea = bounds.removeFromTop(280);
    
    // Three columns
    auto leftColumn = mainArea.removeFromLeft(100);   // IN meter
    auto rightColumn = mainArea.removeFromRight(100); // OUT meter
    auto centerColumn = mainArea;                      // Knobs + Focus
    
    // Store meter bounds for optimized repaint
    inputMeterBounds = leftColumn.reduced(10);
    outputMeterBounds = rightColumn.reduced(10);
    
    // Center: GRAIN + WARM row, FOCUS row
    auto knobRow = centerColumn.removeFromTop(160);
    auto focusRow = centerColumn.removeFromTop(50);
    
    // Footer knob row: INPUT | MIX | OUTPUT
    auto footerKnobArea = bounds.removeFromTop(100);
    
    // Actual footer: version + copyright
    auto footerArea = bounds;
}
```

**Header:**
- Title: "GRAIN" — bold, #1a1a1a
- Subtitle: "MICRO-HARMONIC SATURATION" — tracking-wide, #4a4a4a
- Bypass button: right-aligned

**Footer:**
- Border top: 1px line, rgba(0,0,0,0.1)
- Left: "v1.0.0"
- Right: "Sergio Brocos © 2025"
- Text color: #4a4a4a

**Acceptance criteria:**
- [ ] Layout matches JSX mockup proportions
- [ ] Meters on left and right edges, vertical
- [ ] GRAIN and WARM centered and large
- [ ] FOCUS selector centered below main knobs
- [ ] INPUT, MIX, OUTPUT in bottom row
- [ ] Footer with separator, version, and copyright

---

## Files Summary

| File | Action | Subtask |
|------|--------|---------|
| `Source/UI/GrainLookAndFeel.h` | Create | 1, 2, 4 |
| `Source/UI/GrainLookAndFeel.cpp` | Create | 1, 2, 4 |
| `Source/GrainColours.h` | Modify | 1 |
| `Source/PluginEditor.h` | Modify | 2, 3, 5, 6 |
| `Source/PluginEditor.cpp` | Modify | 1, 2, 3, 4, 5, 6 |
| `GRAIN.jucer` | Modify | 1 |

---

## Global Acceptance Criteria

### Visual
- [ ] Dot-style knobs with correct sizes (L/M/S)
- [ ] Dot arc opening at bottom (standard orientation)
- [ ] Vertical segmented meters with color zones
- [ ] Peak hold visible (~1s hold, then decay)
- [ ] Circular bypass button with LED behavior
- [ ] 3-way Focus switch (radio buttons)
- [ ] Editable value fields (double-click to edit)
- [ ] Layout matches JSX mockup
- [ ] All text legible

### Functionality
- [ ] All 6 parameters + bypass functional
- [ ] Meters respond to audio with peak hold
- [ ] Bypass dims UI to 40% opacity
- [ ] Value fields accept typed input
- [ ] No clicks or artifacts

### Performance
- [ ] Meter repaint optimized (bounds only)
- [ ] CPU usage ≤ Phase A
- [ ] pluginval passes
- [ ] All unit tests pass

---

## Design Decisions (Confirmed)

| Decision | Value | Justification |
|----------|-------|---------------|
| Oversample exposed | **No** | Keep internal per documentation, "safe" philosophy |
| WARM vs WARMTH | **WARM** | More compact visually |
| Focus implementation | **3 TextButtons** | Better UX than ComboBox for 3 options |
| Peak hold | **Yes** | Standard in pro plugins, useful for mixing |
| Editable values | **Yes** | Precision for advanced users |
| Tooltips | **No** | No clear content to add |

---

## Testing Checklist

```bash
# Build
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# Tests
./scripts/run_tests.sh

# Validation
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Visual verification in DAW
# - Check all knob sizes and styling
# - Verify meter animation and peak hold
# - Test bypass toggle and UI dimming
# - Double-click value fields to edit
# - Automate parameters, verify sync
```

---

*Revised based on GrainUI.jsx mockup — GRAIN TFM Project*
