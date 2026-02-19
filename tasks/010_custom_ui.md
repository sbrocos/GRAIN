# TASK 010 — Custom UI from JSX via WebBrowserComponent

**Project:** GRAIN — Micro-harmonic Saturation Processor  
**Status:** Ready  
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
- Implement a **JS ↔ C++ communication bridge** (defined in §5).
- Port all visual components from `GrainUI.jsx`:
  - Dot-style knobs: GRAIN, WARM, INPUT, MIX, OUTPUT
  - Focus switch: LOW / MID / HIGH
  - Bypass button (power icon)
  - VU Meters: IN (L/R) and OUT (L/R), animated
- Connect knobs/switch/bypass to `APVTS` parameters.
- Feed real meter levels from the audio thread to the UI via the bridge.

### 2.2 Out of scope (explicit)
- Oversampling selector (2X / 4X buttons): **removed from UI** — oversampling is internal and not a user decision (per PRD §4.2).
- Preset system (planned for V1.1).
- Advanced animations beyond what the JSX already defines.

---

## 3. UI Components Specification

Derived directly from `GrainUI.jsx`. No visual changes required unless noted.

### 3.1 Knobs

| Label  | Param ID     | Range        | Default | Size   | Unit  |
|--------|--------------|--------------|---------|--------|-------|
| GRAIN  | `grain`      | 0 – 100      | 25      | large  | %     |
| WARM   | `warmth`     | -100 – +100  | 0       | large  | %     |
| INPUT  | `inputGain`  | -12 – +12    | 0       | normal | dB    |
| MIX    | `mix`        | 0 – 100      | 15      | medium | %     |
| OUTPUT | `outputGain` | -12 – +12    | 0       | normal | dB    |

Dot ring: 21–35 dots (size-dependent), active dots in `#d97706` (amber), inactive in `#1a1a1a`.  
Interaction: HTML `<input type="range">` hidden under the knob SVG (as in JSX). No custom drag logic needed in JS.

### 3.2 Focus Switch

Three-segment toggle: LOW / MID / HIGH.  
Param ID: `focus` (0 = Low, 1 = Mid, 2 = High).  
Active segment background: `#d97706`. Inactive: transparent on `#1a1a1a` base.

### 3.3 Bypass Button

Circular, power icon (SVG inline).  
State: active = amber glow (`#d97706`), bypassed = dark (`#1a1a1a`).  
Does **not** map to an `APVTS` parameter — managed as plugin bypass via `AudioProcessor::setBypass()` or equivalent.

### 3.4 VU Meters

Two pairs (IN L/R, OUT L/R). Each bar: 32 segments, vertical, bottom-to-top.  
Segment colors:
- Green `#22c55e` for 0–75%
- Yellow `#eab308` for 75–90%
- Red `#ef4444` for 90–100%

Update rate: ~15 ms (driven by bridge events from audio thread — see §5.3).  
In bypass mode: both meters show near-zero (residual ~5% static level).

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

---

## 4. File Structure

```
GRAIN/
├── Source/
│   ├── PluginEditor.h          ← hosts WebBrowserComponent
│   ├── PluginEditor.cpp
│   └── UI/
│       ├── GrainWebBridge.h    ← NEW: JS↔C++ bridge interface
│       ├── GrainWebBridge.cpp
│       └── Resources/
│           ├── index.html      ← entry point (inlined or loaded via BinaryData)
│           ├── grain-ui.js     ← UI logic (ported from JSX, vanilla JS + CSS)
│           └── grain-ui.css    ← styles (extracted from JSX inline styles)
└── Resources/                  ← Projucer binary resources
    └── UI/                     ← (mirrored from Source/UI/Resources)
```

> **Note on JSX → Vanilla JS:** `GrainUI.jsx` uses React for state management. Since `WebBrowserComponent` renders HTML directly (no React runtime needed), the port replaces:
> - `useState` → plain JS variables + DOM manipulation
> - `useEffect` (meter animation) → `setInterval` in JS, data injected via bridge
> - `Array.from().map()` SVG dots → generated once on init, updated via class toggle

---

## 5. JS ↔ C++ Communication Bridge

This is the most critical architectural decision of the task.

### 5.1 Mechanism: `juce::WebBrowserComponent` Native API (JUCE 7+)

JUCE 7 introduces `WebBrowserComponent::Options` with `withNativeIntegration()` that exposes a native event bus:

```cpp
// C++ side — send event TO the browser
webComponent.emitEventIfBrowserIsVisible("paramChanged", payloadVar);

// C++ side — receive events FROM the browser
webComponent.getNativeEventListener().addListener([this](const juce::var& event) {
    handleBrowserEvent(event);
});
```

```js
// JS side — receive event from C++
window.__JUCE__.backend.addEventListener("paramChanged", (event) => {
    updateKnob(event.paramId, event.value);
});

// JS side — send event to C++
window.__JUCE__.backend.emitEvent("paramChange", { paramId: "grain", value: 42 });
```

> ⚠️ **JUCE version check:** Verify that the project's JUCE version supports `WebBrowserComponent::Options::withNativeIntegration()`. If not (JUCE < 7), fall back to the `evaluateJavascript` + URL scheme approach (see §5.4 fallback).

### 5.2 Event Protocol

All events are JSON objects serialized as `juce::var` / `juce::DynamicObject`.

#### C++ → JS Events

| Event name       | Payload                                      | When                          |
|------------------|----------------------------------------------|-------------------------------|
| `paramChanged`   | `{ paramId: string, value: number }`         | APVTS listener fires          |
| `meterUpdate`    | `{ inL: float, inR: float, outL: float, outR: float }` | ~15ms timer in editor  |
| `bypassChanged`  | `{ bypassed: bool }`                         | Bypass state changes          |
| `initState`      | Full params snapshot (all 6 + bypass)        | Page load complete            |

#### JS → C++ Events

| Event name       | Payload                                      | Action                        |
|------------------|----------------------------------------------|-------------------------------|
| `paramChange`    | `{ paramId: string, value: number }`         | Set APVTS parameter           |
| `bypassChange`   | `{ bypassed: bool }`                         | Toggle plugin bypass          |
| `uiReady`        | `{}`                                         | Triggers `initState` send     |

### 5.3 Meter Update Strategy

VU meter levels come from the **audio thread** and must reach the UI safely:

```
Audio thread → RMS values written to std::atomic<float> (x4)
                         ↓
Editor timer (juce::Timer, ~15ms) reads atomics
                         ↓
emitEventIfBrowserIsVisible("meterUpdate", payload)
                         ↓
JS updates DOM segment colors
```

This avoids any audio-thread → UI direct calls.

### 5.4 Fallback: JUCE < 7 (evaluateJavascript)

If native integration is unavailable:

```cpp
// C++ → JS: inject directly
webComponent.evaluateJavascript("window.grain.onParamChanged('grain', 42)");

// JS → C++: use custom URL scheme
// JS calls: window.location.href = "grain://paramChange?id=grain&value=42"
// C++ overrides: bool pageAboutToLoad(const String& url) { parseGrainScheme(url); return false; }
```

Use this only as fallback — native integration is cleaner and bidirectional.

---

## 6. HTML/JS Porting Guide (JSX → Vanilla)

### 6.1 Knob Component

```js
// JSX Knob → JS class
class DotKnob {
  constructor(container, { label, min, max, defaultValue, unit, size }) {
    this.value = defaultValue;
    this.min = min; this.max = max; this.unit = unit;
    this.dots = []; // SVG circle elements
    this._buildDOM(container, label, size);
    this._buildDots();
    this.setValue(defaultValue);
  }

  setValue(v) {
    this.value = v;
    const norm = (v - this.min) / (this.max - this.min);
    const angle = -135 + norm * 270;
    this.knobBody.style.transform = `rotate(${angle}deg)`;
    const activeDots = Math.round(norm * (this.dots.length - 1));
    this.dots.forEach((dot, i) => {
      dot.setAttribute("fill", i <= activeDots ? "#d97706" : "#1a1a1a");
    });
    this.valueLabel.textContent = v > 0 && this.unit !== "%" ? `+${v}${this.unit}` : `${v}${this.unit}`;
  }

  // Input range onChange → emits paramChange event to C++
  _onInput(e) {
    const v = parseFloat(e.target.value);
    this.setValue(v);
    window.__JUCE__.backend.emitEvent("paramChange", { paramId: this.paramId, value: v });
  }
}
```

### 6.2 Meter Update

```js
// Driven by meterUpdate events from C++
window.__JUCE__.backend.addEventListener("meterUpdate", ({ inL, inR, outL, outR }) => {
  updateMeterBar(inMeterEl, inL, inR);
  updateMeterBar(outMeterEl, outL, outR);
});

function updateMeterBar(el, levelL, levelR) {
  const segments = 32;
  el.querySelectorAll(".seg-l").forEach((seg, i) => {
    seg.style.backgroundColor = getSegColor(i, i < Math.round(levelL * segments), segments);
  });
  // same for R channel
}
```

### 6.3 Init Handshake

```js
// On page load
window.addEventListener("load", () => {
  window.__JUCE__.backend.emitEvent("uiReady", {});
});

window.__JUCE__.backend.addEventListener("initState", (state) => {
  grainKnob.setValue(state.grain);
  warmKnob.setValue(state.warmth);
  inputKnob.setValue(state.inputGain);
  mixKnob.setValue(state.mix);
  outputKnob.setValue(state.outputGain);
  focusSwitch.setValue(state.focus);
  bypassBtn.setState(state.bypassed);
});
```

---

## 7. C++ Integration in PluginEditor

```cpp
class GrainEditor : public juce::AudioProcessorEditor,
                    private juce::Timer,
                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    GrainEditor(GrainProcessor& p)
        : AudioProcessorEditor(p), processor(p),
          webView(juce::WebBrowserComponent::Options{}
                      .withNativeIntegration()
                      .withEventListener("paramChange",    [this](auto& e) { onParamChange(e); })
                      .withEventListener("bypassChange",   [this](auto& e) { onBypassChange(e); })
                      .withEventListener("uiReady",        [this](auto& e) { sendInitState(); }))
    {
        addAndMakeVisible(webView);
        webView.goToURL(getUIResourceURL());   // loads embedded index.html
        processor.apvts.addParameterListener("grain",      this);
        processor.apvts.addParameterListener("warmth",     this);
        // ... all 5 params
        startTimerHz(66); // ~15ms for meter updates
        setSize(480, 520);
    }

private:
    void timerCallback() override
    {
        auto& src = processor.getMeterLevels(); // std::atomic<float>[4]
        juce::DynamicObject payload;
        payload.setProperty("inL",  src[0].load());
        payload.setProperty("inR",  src[1].load());
        payload.setProperty("outL", src[2].load());
        payload.setProperty("outR", src[3].load());
        webView.emitEventIfBrowserIsVisible("meterUpdate", juce::var(&payload));
    }

    void parameterChanged(const juce::String& paramId, float newValue) override
    {
        juce::DynamicObject payload;
        payload.setProperty("paramId", paramId);
        payload.setProperty("value",   newValue);
        webView.emitEventIfBrowserIsVisible("paramChanged", juce::var(&payload));
    }

    void onParamChange(const juce::var& event)
    {
        auto paramId = event["paramId"].toString();
        auto value   = static_cast<float>(event["value"]);
        if (auto* param = processor.apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    void sendInitState()
    {
        juce::DynamicObject state;
        state.setProperty("grain",      getParamValue("grain"));
        state.setProperty("warmth",     getParamValue("warmth"));
        state.setProperty("inputGain",  getParamValue("inputGain"));
        state.setProperty("mix",        getParamValue("mix"));
        state.setProperty("outputGain", getParamValue("outputGain"));
        state.setProperty("focus",      getParamValue("focus"));
        state.setProperty("bypassed",   processor.isBypassed());
        webView.emitEventIfBrowserIsVisible("initState", juce::var(&state));
    }

    juce::WebBrowserComponent webView;
    GrainProcessor& processor;
};
```

---

## 8. Resource Embedding

HTML/CSS/JS files must be embedded into the binary via Projucer's **Binary Resources** panel, accessed at runtime via `BinaryData`:

```cpp
// In PluginEditor — load embedded HTML
juce::String getUIResourceURL()
{
    // Write to temp file on first load (or use data URI)
    auto html = juce::String::fromUTF8(BinaryData::index_html, BinaryData::index_htmlSize);
    // Option A: data URI (simple, no file I/O)
    return "data:text/html;charset=utf-8," + juce::URL::addEscapeChars(html, false);
    // Option B: write to appDataDir/grain_ui/ and return file:// URL
}
```

> **Recommendation:** Use a **temp file approach** (Option B) for development to allow hot-reload during iteration, then switch to the data URI approach for release. The `index.html` can reference `grain-ui.js` and `grain-ui.css` as relative paths when serving from a directory.

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
| 10 | Plugin passes `pluginval --strictness-level 10` | CI/CD pipeline |

---

## 10. Implementation Order

```
Day 1 — Setup
  ├── Add WebBrowserComponent to PluginEditor (replaces native sliders)
  ├── Verify JUCE version supports withNativeIntegration()
  └── Minimal index.html loads and renders inside editor window

Day 2 — Bridge
  ├── Implement GrainWebBridge (event send/receive skeleton)
  ├── uiReady → initState handshake working
  └── Console log events in browser (debug mode)

Day 3 — Knobs + Focus
  ├── Port Knob class from JSX to vanilla JS
  ├── All 5 knobs render with correct dot scales
  └── Knob drag → paramChange → APVTS verified

Day 4 — Meters + Bypass
  ├── Expose MeterLevels atomics from GrainProcessor
  ├── Timer → meterUpdate events → meter DOM updates
  └── Bypass button → bypassChange event → processor bypass

Day 5 — Polish + Edge Cases
  ├── Init state on editor reopen
  ├── APVTS → paramChanged → knob visual sync (DAW automation)
  └── Style QA against JSX reference

Day 6 — Validation
  ├── pluginval run
  ├── Full acceptance criteria checklist
  └── Document known limitations / next steps
```

---

## 11. Risks & Mitigations

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| JUCE version doesn't support `withNativeIntegration()` | Medium | Implement `evaluateJavascript` fallback (§5.4) as parallel path |
| macOS WebKit sandboxing blocks local file access | Low | Use data URI approach for HTML embedding |
| Meter update performance at 66Hz degrades audio | Low | Atomics + timer pattern is decoupled from audio thread |
| Knob feel differs from native JUCE sliders | N/A | JSX already uses `<input type="range">` — identical feel |
| Hot-reload workflow slow during JS iteration | Medium | Serve from localhost during dev (`http://localhost:3000`), switch to embedded for release |

---

## 12. Dev Workflow Tip

During JS development, run the Vite dev server from the original JSX repo and point `GrainEditor` to `http://localhost:5173` instead of the embedded resource. This enables **hot module reload** without recompiling the plugin.

```cpp
#if JUCE_DEBUG
    webView.goToURL("http://localhost:5173");  // Vite dev server
#else
    webView.goToURL(getEmbeddedResourceURL()); // Production binary
#endif
```

---

*TASK 010 — GRAIN project — Master Desarrollo con IA*
