# Task 010: Custom UI (Phase B)

## Objective

Replace the functional Phase A editor with a polished custom UI. Phase A established structure and connectivity (standard JUCE sliders/buttons). Phase B adds visual refinement: custom knob graphics, segmented meters, LED bypass indicator, refined typography, and optimized repaint. The standalone transport bar and waveform display also receive visual polish to match.

**Constraint:** No parameter, DSP, or layout changes. Same 500 x 350px (VST3) / 500 x 520px (Standalone). Same 7 parameters. Visual-only transformation.

---

## Prerequisites

- Task 008 completed (Phase A — functional layout)
- Task 009b completed (Standalone file player, transport bar, waveform)

---

## Design Principles

1. **Minimal and safe UX** — No new controls, no hidden features, no cognitive load
2. **Dark, warm aesthetic** — Consistent with the existing GrainColours palette (stone/amber)
3. **Analog-inspired** — Subtle textures, warm colors, no skeuomorphism
4. **Readable** — Labels and values legible at 500px width
5. **Performance** — Meter repaint restricted to meter areas only (not full editor)

---

## Subtasks

### Subtask 1: GrainLookAndFeel + Custom Knobs

**Objective:** Create a custom `LookAndFeel` class and replace the default JUCE rotary slider rendering with custom-painted knobs.

**Files to create:**
- `Source/UI/GrainLookAndFeel.h`
- `Source/UI/GrainLookAndFeel.cpp`

**Files to modify:**
- `Source/PluginEditor.h` — Add `GrainLookAndFeel` member (declared before all components)
- `Source/PluginEditor.cpp` — Apply LookAndFeel in constructor, clear in destructor
- `GRAIN.jucer` — Add `Source/UI/` group
- `GRAINTests.jucer` — Add `Source/UI/` group (if needed for compilation)

**Scope:**

`GrainLookAndFeel` extends `juce::LookAndFeel_V4`:
- `drawRotarySlider()` — Custom knob with filled arc + dot indicator
- `drawButtonBackground()` / `drawButtonText()` — Rounded rect buttons with GrainColours
- `drawComboBox()` / `drawPopupMenuItem()` — Styled Focus selector
- `drawLabel()` — Consistent label styling
- Default font via `FontOptions` (JUCE 8 API)

**Knob design spec:**

| Element | Spec |
|---------|------|
| Background arc | Full circle, `kSurface` color, 3px stroke |
| Value arc | Partial arc (min to current), `kAccent` color, 3px stroke |
| Dot indicator | Small filled circle on the arc at current value position |
| Value text | Below knob, current numeric value |
| Label | Above knob, parameter name |

**Big knobs** (GRAIN, WARMTH): ~100px diameter
**Small knobs** (INPUT, MIX, OUTPUT): ~60px diameter

No images/bitmaps. Pure `juce::Graphics` drawing (resolution-independent).

**Technical notes:**

```cpp
// LookAndFeel lifecycle — CRITICAL ordering
// In PluginEditor constructor (BEFORE setSize):
setLookAndFeel(&grainLookAndFeel);

// In PluginEditor destructor (BEFORE any component cleanup):
setLookAndFeel(nullptr);
```

```cpp
void GrainLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    // 1. Calculate arc geometry from bounds
    // 2. Draw background arc (full range, kSurface)
    // 3. Draw value arc (min to current, kAccent)
    // 4. Draw dot indicator at current position
}
```

**Acceptance criteria:**
- [ ] All 5 rotary sliders render with custom arc + dot style
- [ ] Big knobs visually larger than small knobs
- [ ] Buttons (Bypass, transport) render with rounded style
- [ ] Focus ComboBox styled consistently
- [ ] Labels and text consistent with GrainColours
- [ ] pluginval still passes
- [ ] All unit tests still pass

---

### Subtask 2: Segmented Meters + Peak Hold

**Objective:** Replace solid-fill meter bars with segmented LED-style meters and add peak hold indicators.

**Files to modify:**
- `Source/PluginEditor.h` — Add `PeakHold` struct, meter bounds members
- `Source/PluginEditor.cpp` — Rewrite `drawMeter()`, optimize `timerCallback()` repaint

**Scope:**

**Meter design spec:**

| Element | Spec |
|---------|------|
| Segments | 12-16 horizontal bars per channel, 2px gap between |
| Green zone | Bottom 70% of range (-60 to -18 dB) |
| Yellow zone | 70%-90% (-18 to -6 dB) |
| Red zone | Top 10% (-6 to 0 dB) |
| Peak hold | Top segment stays lit for ~1s after peak, then fades |
| Background | Dark segments visible even when silent |

**Repaint optimization:**
- Current Phase A: full `repaint()` at 30 FPS
- Subtask 2: `repaint(inputMeterBounds)` and `repaint(outputMeterBounds)` only
- Store bounds as `juce::Rectangle<int>` members computed once in `resized()`

**Peak hold implementation:**

```cpp
struct PeakHold
{
    float peakLevel = 0.0f;
    int holdCounter = 0;  // frames remaining at 30 FPS

    void update(float newLevel)
    {
        if (newLevel >= peakLevel)
        {
            peakLevel = newLevel;
            holdCounter = 30;  // ~1 second at 30 FPS
        }
        else if (holdCounter > 0)
        {
            --holdCounter;
        }
        else
        {
            peakLevel *= 0.95f;  // Fade after hold
        }
    }
};
```

**Acceptance criteria:**
- [ ] Meters render with segmented bars (not solid fill)
- [ ] Green/yellow/red color zones correct
- [ ] Peak hold visible on transient material (~1s hold)
- [ ] Silent state shows dark segments (not blank)
- [ ] `timerCallback()` only repaints meter bounds (not full editor)
- [ ] CPU usage same or lower than Phase A
- [ ] No visual glitches on rapid level changes

---

### Subtask 3: LED Bypass Indicator

**Objective:** Replace the `TextButton` bypass toggle with a visual LED indicator.

**Files to modify:**
- `Source/PluginEditor.h` — Replace or wrap bypass button with LED component
- `Source/PluginEditor.cpp` — Implement LED drawing and bypass connection
- `Source/GrainColours.h` — Add `kLedGreen`, `kLedRed` (if not added in Subtask 1)

**Scope:**

| State | Visual |
|-------|--------|
| Active (processing) | Green LED dot + "ON" label |
| Bypassed | Red/dim LED dot + "BYPASS" label |
| Transition | Smooth color interpolation (~100ms) |

Still connected to the `bypass` `AudioParameterBool` via APVTS `ButtonAttachment`.

**Color additions:**

```cpp
inline const juce::Colour kLedGreen{0xff4ade80};   // Bypass LED on
inline const juce::Colour kLedRed{0xffef4444};      // Bypass LED off
```

**Acceptance criteria:**
- [ ] LED shows green dot when processing (bypass = false)
- [ ] LED shows red/dim dot when bypassed (bypass = true)
- [ ] Clicking toggles state (same APVTS connection)
- [ ] Transition is smooth (no hard pop)
- [ ] Label text changes: "ON" / "BYPASS"

---

### Subtask 4: Header + Footer Polish

**Objective:** Add subtle visual refinement to the header and footer areas.

**Files to modify:**
- `Source/PluginEditor.cpp` — Update `paint()` for header/footer areas
- `Source/GrainColours.h` — Add `kSurfaceLight` if needed

**Scope:**

**Header:**

| Element | Current (Phase A) | Phase B |
|---------|-------------------|---------|
| Title | `drawText("GRAIN")` 24pt bold | Custom font weight, letter-spacing |
| Background | Flat `kSurface` fill | Subtle gradient (top: slightly lighter) |
| Separator | None | 1px line at bottom, `kAccentDim` |

**Footer:**

| Element | Current (Phase A) | Phase B |
|---------|-------------------|---------|
| Background | Flat `kSurface` fill | Match header treatment |
| Separator | None | 1px line at top, `kAccentDim` |

**Color additions (if needed):**

```cpp
inline const juce::Colour kSurfaceLight{0xff3c3836};  // Hover/highlight, gradient top
```

**Acceptance criteria:**
- [ ] Header has subtle gradient background
- [ ] 1px separator at header bottom (kAccentDim)
- [ ] 1px separator at footer top (kAccentDim)
- [ ] Footer gradient matches header style
- [ ] Title text visually refined
- [ ] No layout changes (same pixel dimensions)

---

### Subtask 5: Standalone Transport & Waveform Polish

**Objective:** Apply consistent visual styling to the standalone-specific transport bar and waveform display.

**Files to modify:**
- `Source/Standalone/TransportBar.cpp` — Visual polish (buttons styled via LookAndFeel, progress bar, separators)
- `Source/Standalone/WaveformDisplay.cpp` — Grid lines, refined colors, empty state text

**Scope:**

**Transport bar:**

| Element | Current | Phase B |
|---------|---------|---------|
| Buttons | Default `TextButton` | Styled via `GrainLookAndFeel` — rounded, hover states |
| Progress bar | Basic painted rect | Rounded track, `kAccent` fill |
| Time display | Plain `drawText` | Monospaced font, consistent sizing |
| Background | Flat `kBackground` | Subtle separator line at top |

**Waveform display:**

| Element | Current | Phase B |
|---------|---------|---------|
| Dry waveform | Semi-transparent default | `kText` at 30% opacity |
| Wet waveform | Solid columns | `kAccent` at 60% opacity |
| Cursor | Vertical line | `kTextBright`, 1px |
| Background | Flat black | `kBackground` with subtle grid lines (0dB and time markers) |
| Empty state | Blank | "Drop audio file here" text, centered, `kText` at 40% opacity |

**Acceptance criteria:**
- [ ] Transport buttons render with rounded LookAndFeel style
- [ ] Progress bar has rounded track
- [ ] Time display uses monospaced font
- [ ] Waveform has grid lines at 0dB reference
- [ ] Empty waveform shows "Drop audio file here" hint
- [ ] Dry/wet waveform colors match spec
- [ ] All standalone functionality still works (play, stop, seek, loop, export, drag & drop)

---

## Color Palette (Phase B Additions)

Keep existing `GrainColours` namespace. Add only what is needed:

```cpp
namespace GrainColours
{
    // Existing 9 colors (unchanged)
    // ...

    // Phase B additions
    inline const juce::Colour kSurfaceLight{0xff3c3836};  // Hover/highlight, gradient
    inline const juce::Colour kLedGreen{0xff4ade80};      // Bypass LED on
    inline const juce::Colour kLedRed{0xffef4444};        // Bypass LED off
}
```

---

## Files Summary

| File | Action | Subtask |
|------|--------|---------|
| `Source/UI/GrainLookAndFeel.h` | Create | 1 |
| `Source/UI/GrainLookAndFeel.cpp` | Create | 1 |
| `Source/GrainColours.h` | Modify | 1, 3, 4 |
| `Source/PluginEditor.h` | Modify | 1, 2, 3 |
| `Source/PluginEditor.cpp` | Modify | 1, 2, 3, 4 |
| `Source/Standalone/TransportBar.cpp` | Modify | 5 |
| `Source/Standalone/WaveformDisplay.cpp` | Modify | 5 |
| `GRAIN.jucer` | Modify | 1 |
| `GRAINTests.jucer` | Modify | 1 |

---

## Acceptance Criteria (Global)

### Visual
- [ ] Custom knobs render correctly (arc + dot indicator)
- [ ] Big knobs visually larger than small knobs
- [ ] Segmented meters with green/yellow/red zones
- [ ] Peak hold visible on transient material
- [ ] LED bypass indicator toggles between green and red/dim
- [ ] Header and footer have subtle gradients and separators
- [ ] Focus selector styled consistently
- [ ] Standalone transport buttons styled (rounded, hover states)
- [ ] Standalone waveform has grid lines and empty state text
- [ ] All text legible at native window size

### Functionality (No Regressions)
- [ ] All 7 parameters still connected and functional
- [ ] Meters still respond to audio
- [ ] Bypass still works
- [ ] Standalone file player, transport, waveform, export all work
- [ ] Drag & drop still works
- [ ] No clicks or artifacts from UI changes

### Performance
- [ ] Meter repaint restricted to meter bounds (not full editor)
- [ ] CPU usage same or lower than Phase A
- [ ] pluginval still passes
- [ ] All unit tests pass (95)

### Quality
- [ ] No deprecation warnings (JUCE 8 FontOptions API)
- [ ] No new compiler warnings
- [ ] clang-format clean

---

## Testing Checklist

```bash
# 1. Build VST3
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 2. Build Standalone
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" -configuration Debug build

# 3. Run unit tests
./scripts/run_tests.sh

# 4. Run pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 5. Visual verification in DAW (Logic Pro / Ableton)
# - Verify knobs, meters, bypass, labels render correctly
# - Automate parameters — verify smooth visual response
# - Resize host window — verify no layout glitches

# 6. Visual verification in Standalone
# - Open file, play, verify waveform + transport styling
# - Drag & drop file, verify border feedback
# - Export, verify transport bar states
```

---

## Branching Strategy

```
main
 └── feature/010-custom-ui                      ← main task branch
      ├── feature/010-custom-ui/001-lookandfeel  ← subtask 1
      ├── feature/010-custom-ui/002-meters       ← subtask 2
      ├── feature/010-custom-ui/003-bypass-led   ← subtask 3
      ├── feature/010-custom-ui/004-header-footer ← subtask 4
      └── feature/010-custom-ui/005-standalone   ← subtask 5
```

Each subtask branch merges into `feature/010-custom-ui` via PR.
Final PR goes from `feature/010-custom-ui` → `main`.

---

## Out of Scope

- Custom fonts / embedded typefaces (use system fonts)
- Animations beyond LED transition and peak hold fade
- Resizable window / scalable UI
- Bitmap/SVG assets
- Tooltip popups
- Preset browser UI
- A/B switch UI
