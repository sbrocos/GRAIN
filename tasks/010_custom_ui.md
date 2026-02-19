# TASK 010 — Custom UI from JSX via WebBrowserComponent

**Project:** GRAIN — Micro-harmonic Saturation Processor
**Status:** In Progress
**Priority:** High
**Depends on:** TASK 008 (PluginEditor base), TASK 007 (Oversampling integration)
**Estimated effort:** 4–6 days

---

## 1. Goal

Replace the placeholder JUCE native editor (`GrainEditor` with basic sliders) with a polished HTML/CSS/JS UI derived from `GrainUI.jsx`, embedded and served via `juce::WebBrowserComponent`.

The result must be **visually identical to the JSX mockup** (dot-style knobs, VU meters, Focus switch, Bypass button) and **bidirectionally synchronized** with the JUCE parameter system (`APVTS`).

---

## 2. Scope

### 2.1 In scope
- Embed the HTML/CSS/JS UI as binary resources inside the plugin binary.
- Render the UI via `juce::WebBrowserComponent` inside `GrainEditor`.
- Implement parameter sync using **JUCE 8 relay pattern** (see §5).
- Port all visual components from `GrainUI.jsx`:
  - Dot-style knobs: GRAIN, WARM, INPUT, MIX, OUTPUT
  - Focus switch: LOW / MID / HIGH
  - Bypass button (power icon)
  - VU Meters: IN (L/R) and OUT (L/R), animated
- Connect knobs/switch/bypass to `APVTS` parameters via relay attachments.
- Feed real meter levels from the audio thread to the UI via custom events.
- **Standalone mode**: Keep native JUCE components (TransportBar, WaveformDisplay, AudioRecorder, drag & drop) alongside the WebBrowserComponent.

### 2.2 Out of scope (explicit)
- Oversampling selector (2X / 4X buttons): **removed from UI** — oversampling is internal and not a user decision (per PRD §4.2).
- Preset system (planned for V1.1).
- Advanced animations beyond what the JSX already defines.

---

## 3. UI Components Specification

Derived directly from `GrainUI.jsx`. No visual changes required unless noted.

### 3.1 Knobs

| Label  | APVTS ID     | APVTS Range  | Display Range | Size   | Unit  | Relay Name       |
|--------|-------------|--------------|---------------|--------|-------|-----------------|
| GRAIN  | `drive`     | 0.0–1.0      | 0–100%        | large  | %     | `grainSlider`   |
| WARM   | `warmth`    | 0.0–1.0      | -100 to +100% | large  | %     | `warmSlider`    |
| INPUT  | `inputGain` | -12 to +12   | -12 to +12 dB | normal | dB    | `inputSlider`   |
| MIX    | `mix`       | 0.0–1.0      | 0–100%        | medium | %     | `mixSlider`     |
| OUTPUT | `output`    | -12 to +12   | -12 to +12 dB | normal | dB    | `outputSlider`  |

Dot ring: 21–35 dots (size-dependent), active dots in `#d97706` (amber), inactive in `#1a1a1a`.
Interaction: HTML `<input type="range">` hidden under the knob SVG (as in JSX). Range input works with normalized 0–1 values; display values computed by JS `displayFn`.

### 3.2 Focus Switch

Three-segment toggle: LOW / MID / HIGH.
APVTS ID: `focus` (AudioParameterChoice: 0 = Low, 1 = Mid, 2 = High).
Relay: `WebComboBoxRelay("focusCombo")` + `WebComboBoxParameterAttachment`.
Active segment background: `#d97706`. Inactive: transparent on `#1a1a1a` base.

### 3.3 Bypass Button

Circular, power icon (SVG inline).
State: active = amber glow (`#d97706`), bypassed = dark (`#1a1a1a`).
APVTS ID: `bypass` (AudioParameterBool). Managed via `WebToggleButtonRelay("bypassToggle")` + `WebToggleButtonParameterAttachment`.
When bypassed, main controls area dims to 40% opacity.

### 3.4 VU Meters

Two pairs (IN L/R, OUT L/R). Each bar: 32 segments, vertical, bottom-to-top.
Segment colors:
- Green `#22c55e` for 0–75%
- Yellow `#eab308` for 75–90%
- Red `#ef4444` for 90–100%

Update rate: ~16 ms (60 Hz timer, driven by `emitEventIfBrowserIsVisible("meterUpdate", ...)` custom event).
Meter decay applied in C++ (`kMeterDecay = 0.85f`) before sending to JS.

### 3.5 Layout

```
┌────────────────────────────────────────┐
│  GRAIN        [header + bypass btn]    │
│  MICRO-HARMONIC SATURATION             │
├──────────────────────────────────────  │
│ [IN meter] [GRAIN knob] [WARM knob] [OUT meter] │
│            [FOCUS switch LOW/MID/HIGH] │
├────────────────────────────────────────│
│ [INPUT knob]   [MIX knob]  [OUTPUT knob] │
├────────────────────────────────────────│
│  v1.0.0              Sergio Brocos © 2025 │
└────────────────────────────────────────┘
```

Background: `#B6C1B9` (sage green panel). Outer: `#8a9a91`.
Window size: **480 × 480** (plugin), **480 × 650** (standalone with transport + waveform).

---

## 4. File Structure

```
GRAIN/
├── Source/
│   ├── PluginEditor.h          ← hosts WebBrowserComponent + relays + attachments
│   ├── PluginEditor.cpp
│   └── UI/
│       ├── GrainLookAndFeel.h  ← kept (standalone native components)
│       ├── GrainLookAndFeel.cpp
│       └── Resources/
│           ├── index.html      ← entry point (served via ResourceProvider)
│           ├── grain-ui.js     ← UI logic (relay state adapters + components)
│           └── grain-ui.css    ← styles (extracted from JSX inline styles)
```

> **Note:** No separate `GrainWebBridge.h/cpp` is needed. The relay + attachment pattern handles all parameter sync automatically. Bridge logic is embedded directly in `PluginEditor`.

> **Note on JSX → Vanilla JS:** `GrainUI.jsx` uses React for state management. The port replaces:
> - `useState` → `SliderState`/`ToggleState`/`ComboBoxState` relay adapters
> - `useEffect` (meter animation) → `window.__JUCE__.backend.addEventListener("meterUpdate", ...)`
> - `Array.from().map()` SVG dots → generated once on init, updated via `setAttribute("fill", ...)`

---

## 5. JS ↔ C++ Communication Bridge

### 5.1 Architecture: JUCE 8 Relay Pattern

Instead of a custom event protocol, we use **JUCE 8's built-in relay classes** for parameter synchronization. This is the official JUCE approach (demonstrated in `JUCE/examples/Plugins/WebViewPluginDemo.h`).

**C++ side** (in `PluginEditor`):

```cpp
// 1. Declare relays (members, before webView)
juce::WebSliderRelay       driveSliderRelay  { "grainSlider" };
juce::WebToggleButtonRelay bypassToggleRelay { "bypassToggle" };
juce::WebComboBoxRelay     focusComboRelay   { "focusCombo" };

// 2. Build WebBrowserComponent options chaining .withOptionsFrom()
SinglePageBrowser webView { juce::WebBrowserComponent::Options{}
    .withNativeIntegrationEnabled()
    .withOptionsFrom(driveSliderRelay)
    .withOptionsFrom(bypassToggleRelay)
    .withOptionsFrom(focusComboRelay)
    .withResourceProvider([this](const auto& url) { return getResource(url); })
};

// 3. Create parameter attachments (members, after webView)
juce::WebSliderParameterAttachment driveAttachment {
    *apvts.getParameter("drive"), driveSliderRelay
};
```

**JS side** (vanilla, no build step — relay adapters replicated from JUCE's `index.js`):

```js
// SliderState adapter talks to relay via __juce__slider<name> events
class SliderState {
    constructor(name) {
        this.identifier = "__juce__slider" + name;
        window.__JUCE__.backend.addEventListener(this.identifier, (e) => this._handle(e));
    }
    setNormalisedValue(norm) { /* emit valueChanged event */ }
    getNormalisedValue() { /* compute from scaled value */ }
    sliderDragStarted() { /* emit sliderDragStarted event */ }
    sliderDragEnded() { /* emit sliderDragEnded event */ }
}
```

### 5.2 Why Relays Over Custom Events

| Feature | Relay Pattern | Custom Events |
|---------|:---:|:---:|
| Automatic APVTS sync | Yes | Manual |
| Undo manager support | Yes | Manual |
| DAW gesture tracking (drag start/end) | Yes | Manual |
| Parameter properties (range, skew, label) sent to JS | Yes | Manual |
| DAW parameter hover highlighting | Yes | No |
| No separate bridge class needed | Yes | No |

### 5.3 Meter Update Strategy (Custom Event)

VU meters are **not** parameters — they use a custom event, not relays:

```
Audio thread → RMS values written to std::atomic<float> (x4)
                         ↓
Editor timer (juce::Timer, 60 Hz) reads atomics + applies decay
                         ↓
emitEventIfBrowserIsVisible("meterUpdate", {inL, inR, outL, outR})
                         ↓
JS updates DOM segment colors
```

### 5.4 Resource Serving (ResourceProvider)

HTML/CSS/JS files are embedded as BinaryData via Projucer (`resource="1"`) and served through `WebBrowserComponent::ResourceProvider`:

```cpp
std::optional<juce::WebBrowserComponent::Resource>
getResource(const juce::String& url) {
    // url == "/" → serve index.html
    // url == "/grain-ui.js" → serve grain-ui.js
    // Reads from BinaryData::index_html, BinaryData::grainui_js, etc.
}
```

Navigate to: `webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot())`.

On macOS this resolves to `juce://juce.backend/`.

---

## 6. HTML/JS Porting Guide (JSX → Vanilla)

### 6.1 Knob Component

```js
class DotKnob {
    constructor(container, { relayName, label, size, displayFn }) {
        this.state = new SliderState(relayName);
        this._buildDOM(container, label, size);
        this.state.addValueListener(() => this._updateVisual());
        this.state.requestInitialUpdate();
    }

    _updateVisual() {
        const norm = this.state.getNormalisedValue();
        const angle = -135 + norm * 270;
        this.knobBody.style.transform = `rotate(${angle}deg)`;
        // Update dots, value label...
    }
}
```

### 6.2 Meter Update

```js
window.__JUCE__.backend.addEventListener("meterUpdate", (data) => {
    inMeter.update(data.inL, data.inR);
    outMeter.update(data.outL, data.outR);
});
```

### 6.3 Focus Switch

```js
class FocusSwitch {
    constructor(container) {
        this.state = new ComboBoxState("focusCombo");
        // Build LOW/MID/HIGH buttons
        this.state.addValueListener(() => this._updateVisual());
        this.state.requestInitialUpdate();
    }
}
```

### 6.4 Bypass Button

```js
class BypassButton {
    constructor(container) {
        this.state = new ToggleState("bypassToggle");
        // Build power icon SVG button
        this.state.addValueListener(() => this._updateVisual());
        this.state.requestInitialUpdate();
    }
}
```

---

## 7. C++ Integration in PluginEditor

```cpp
class GRAINAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::FileDragAndDropTarget,     // standalone drag & drop
    public FilePlayerSource::Listener,       // standalone export workflow
    private juce::Timer,                     // meter updates
    private TransportBar::Listener           // standalone transport
{
    // === Relays (declared BEFORE webView) ===
    juce::WebSliderRelay       driveSliderRelay  {"grainSlider"};
    juce::WebSliderRelay       warmthSliderRelay {"warmSlider"};
    juce::WebSliderRelay       inputSliderRelay  {"inputSlider"};
    juce::WebSliderRelay       mixSliderRelay    {"mixSlider"};
    juce::WebSliderRelay       outputSliderRelay {"outputSlider"};
    juce::WebToggleButtonRelay bypassToggleRelay {"bypassToggle"};
    juce::WebComboBoxRelay     focusComboRelay   {"focusCombo"};

    // === WebView (declared AFTER relays) ===
    SinglePageBrowser webView;  // Options built with withOptionsFrom() for all relays

    // === Attachments (declared AFTER webView) ===
    juce::WebSliderParameterAttachment       driveAttachment;
    juce::WebSliderParameterAttachment       warmthAttachment;
    juce::WebSliderParameterAttachment       inputAttachment;
    juce::WebSliderParameterAttachment       mixAttachment;
    juce::WebSliderParameterAttachment       outputAttachment;
    juce::WebToggleButtonParameterAttachment bypassAttachment;
    juce::WebComboBoxParameterAttachment     focusAttachment;

    // === Timer for meters ===
    void timerCallback() override {
        // Read atomics, apply decay, emitEventIfBrowserIsVisible("meterUpdate", ...)
    }

    // === Standalone members (unchanged from Phase A) ===
    // FilePlayerSource, TransportBar, WaveformDisplay, AudioRecorder, drag & drop
};
```

**Declaration order is critical**: Relays → webView → attachments (C++ destructor order).

---

## 8. Resource Embedding

HTML/CSS/JS files are added to `GRAIN.jucer` under `Source/UI/Resources/` with `resource="1"`:

```xml
<GROUP name="Resources">
    <FILE id="IndexHtml" name="index.html" compile="0" resource="1"
          file="Source/UI/Resources/index.html"/>
    <FILE id="GrainUiJs" name="grain-ui.js" compile="0" resource="1"
          file="Source/UI/Resources/grain-ui.js"/>
    <FILE id="GrainUiCss" name="grain-ui.css" compile="0" resource="1"
          file="Source/UI/Resources/grain-ui.css"/>
</GROUP>
```

Projucer generates `BinaryData::index_html`, `BinaryData::grainui_js`, `BinaryData::grainui_css`.

---

## 9. Acceptance Criteria

| # | Criterion | How to verify |
|---|-----------|---------------|
| 1 | UI renders inside plugin window with correct colors/layout | Visual inspection in DAW |
| 2 | Knob drag updates APVTS parameter | DAW automation lane shows change |
| 3 | APVTS change (DAW automation) updates knob visually | Record automation, play back |
| 4 | Focus switch sets correct `focus` param value (0/1/2) | Parameter monitor in DAW |
| 5 | Bypass button disables processing and dims UI | Bypass reveal listening test |
| 6 | VU meters animate with real signal levels | Route audio through plugin |
| 7 | Meters go near-zero in bypass | Visual check |
| 8 | Reloading the plugin window restores correct param state | Close/reopen editor |
| 9 | No oversampling controls visible in UI | Visual inspection |
| 10 | Plugin passes `pluginval --skip-gui-tests` | CI/CD pipeline |
| 11 | Standalone: transport, waveform, recorder, drag & drop work | Manual test |
| 12 | All 95 unit tests still pass | `./scripts/run_tests.sh` |

---

## 10. Implementation Order

```
Step 1 — HTML/CSS/JS Resources
  ├── Create index.html (structural divs)
  ├── Create grain-ui.css (all styles from JSX)
  └── Create grain-ui.js (relay adapters + DotKnob + MeterBar + FocusSwitch + BypassButton)

Step 2 — PluginEditor Rewrite
  ├── PluginEditor.h: relays + SinglePageBrowser + attachments + meter members
  └── PluginEditor.cpp: constructor (Options), resource provider, timerCallback, layout

Step 3 — GRAIN.jucer Update
  ├── Add 3 resource files with resource="1"
  └── Projucer resave (GRAINTests first, then GRAIN)

Step 4 — Build & Validate
  ├── Build VST3 + Standalone
  ├── Run unit tests (95 tests)
  ├── Run pluginval
  └── Visual QA against JSX reference
```

---

## 11. Risks & Mitigations

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| BinaryData naming (hyphens → underscores) | Low | Verified: `grainui_js`, `grainui_css`, `index_html` |
| Hidden `<input type="range">` feels wrong vs rotary knob | Medium | Can implement custom JS mouse drag for vertical-to-rotation mapping |
| WebBrowserComponent consumes drag events in standalone | Low | `FileDragAndDropTarget` is on the editor, not the webView |
| Relay `requestInitialUpdate` timing | Low | Relay system handles this automatically via `sendInitialUpdate()` |
| Meter emission thread safety | None | `emitEventIfBrowserIsVisible` called from Timer (message thread) |

---

## 12. Dev Workflow Tip

During JS development, use the ResourceProvider's `allowedOriginIn` parameter to allow a localhost dev server:

```cpp
.withResourceProvider([this](const auto& url) { return getResource(url); },
                      "http://localhost:5173")  // Vite dev server origin
```

Then navigate to `http://localhost:5173` instead of `getResourceProviderRoot()` during development.

---

*TASK 010 — GRAIN project — Master Desarrollo con IA*
