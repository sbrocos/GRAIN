# GRAIN — Context for Claude Code

## Project Overview

GRAIN is a micro-harmonic saturation processor designed to enrich digital signals in a subtle, controlled, and stable way. It increases perceived density and coherence without introducing obvious distortion or altering transients, tonal balance, or stereo image.

**Deliverables:** VST3 plugin + Standalone application for macOS Apple Silicon (ARM64)

## Tech Stack

| Component | Technology |
|-----------|------------|
| Framework | JUCE 8.x |
| Language | C++17 |
| Build | Xcode (via Projucer) |
| Target | macOS ARM64 (Apple Silicon) |
| Formats | VST3, Standalone |

## Repository Structure

```
GRAIN/
├── docs/                    # Project documentation
│   ├── DELIVERABLE_EN.md    # PRD / Academic deliverable
│   ├── DEVELOPMENT_ENVIRONMENT.md
│   ├── DSP_ARCHITECTURE.md
│   ├── TESTING.md           # Testing strategy (pluginval, unit, listening)
│   └── GRAIN_Code_Architecture.md
├── tasks/                   # Implementation tasks (numbered)
├── Source/
│   ├── DSP/
│   │   ├── CalibrationConfig.h   # Centralized DSP calibration constants
│   │   ├── DSPHelpers.h         # Pure utility functions (calculateCoefficient, applyMix, applyGain)
│   │   ├── RMSDetector.h        # RMS envelope follower (stateful)
│   │   ├── DynamicBias.h        # Asymmetric bias function (pure)
│   │   ├── Waveshaper.h         # tanh waveshaper (pure)
│   │   ├── WarmthProcessor.h    # Warmth/asymmetry function (pure)
│   │   ├── DCBlocker.h          # DC blocking filter (stateful, mono)
│   │   ├── SpectralFocus.h      # Biquad shelf EQ for spectral focus (stateful, mono)
│   │   └── GrainDSPPipeline.h   # Per-channel DSP pipeline orchestrator
│   ├── Tests/
│   │   ├── TestMain.cpp         # Console app entry point for test runner
│   │   ├── DSPTests.cpp         # Unit tests (per-module)
│   │   ├── PipelineTest.cpp     # Integration tests (full pipeline)
│   │   ├── OversamplingTest.cpp # Oversampling unit tests
│   │   └── CalibrationTest.cpp  # CalibrationConfig unit tests
│   ├── PluginProcessor.h    # Audio processing logic
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h       # GUI
│   └── PluginEditor.cpp
├── scripts/
│   ├── format.sh            # clang-format wrapper
│   └── run_tests.sh         # Build & run all unit tests (CI-friendly)
├── GRAIN.jucer              # Projucer project (VST3 + Standalone)
├── GRAINTests.jucer         # Projucer project (ConsoleApp test runner)
└── CLAUDE.md                # This file
```

## DSP Architecture

### Current Pipeline (with oversampling)
```
Input → Input Gain → [Upsample] → Dynamic Bias → tanh Waveshaper → Warmth → Focus → [Downsample] → Mix (dry/wet) → DC Blocker → Output Gain
```

- Nonlinear stages (Bias → Waveshaper → Warmth → Focus) run at **oversampled rate**
- Linear stages (Mix → DC Blocker → Gain) run at **original rate**
- 2× oversampling in real-time, 4× in offline bounce (`isNonRealtime()`)

### DSP Module Architecture

All DSP modules live in `Source/DSP/` as individual header files. Each module is either:
- **Pure function** (stateless): `Waveshaper.h`, `DynamicBias.h`, `WarmthProcessor.h`, `DSPHelpers.h`
- **Stateful struct** (mono, one instance per channel): `DCBlocker.h`, `RMSDetector.h`, `SpectralFocus.h`
- **Configuration**: `CalibrationConfig.h` — centralized DSP calibration constants

The `GrainDSPPipeline.h` orchestrates the full chain as a mono `DSPPipeline` struct.
Stereo is managed by `PluginProcessor` creating two pipeline instances (L/R).

**Bypass:** Implemented via `AudioParameterBool` + mix smoothing (bypass ON → mix target = 0).

## MVP Parameters

| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `drive` | Drive | 0.0–1.0 | 0.5 | Maps to 1x–4x gain (UI label: "GRAIN") |
| `mix` | Mix | 0.0–1.0 | 0.2 | 0 = dry, 1 = wet |
| `output` | Output | -12 to +12 dB | 0.0 | Final level trim |
| `warmth` | Warmth | 0.0–1.0 | 0.0 | Asymmetric even harmonics |
| `bypass` | Bypass | bool | false | AudioParameterBool, via mix smoothing |
| `inputGain` | Input Gain | -12 to +12 dB | 0.0 | Pre-saturation level trim |
| `focus` | Focus | Low/Mid/High | Mid | Spectral focus shelf EQ (AudioParameterChoice) |

## Design Principles

1. **Transparency:** No perceptible changes at 10–20% wet
2. **Stability:** No artifacts, no wobble on sustained material
3. **Safety:** No auto-gain, no hidden behavior
4. **Simplicity:** Minimal UI, no technical decisions for user

## Testing Strategy

### Three-Layer Approach

| Layer | Tool | What it catches |
|-------|------|-----------------|
| Automated validation | pluginval | Crashes, memory leaks, VST3 compliance |
| Unit tests | JUCE UnitTest | DSP math correctness |
| Listening tests | Manual | Sonic quality, transparency, artifacts |

### Test Constants
```cpp
namespace TestConstants
{
    constexpr float TOLERANCE = 1e-5f;
    constexpr float CLICK_THRESHOLD = 0.01f;  // -40dB
    constexpr int BUFFER_SIZE = 512;
}
```

### pluginval

```bash
# Use app bundle path (symlink in /usr/local/bin may not work)
PLUGINVAL=/Applications/pluginval.app/Contents/MacOS/pluginval

# Basic validation (during development)
$PLUGINVAL --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Strict validation (before release)
$PLUGINVAL --skip-gui-tests --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

### Unit Tests

Tests run via a separate console application (`GRAINTests.jucer`).
The Xcode scheme name `"GRAINTests - ConsoleApp"` is auto-generated by Projucer
(pattern: `<ProjectName> - <TargetType>`).

```bash
# Quick: build + run via script
./scripts/run_tests.sh

# Manual: build test runner, then execute
xcodebuild -project Builds/MacOSX-Tests/GRAINTests.xcodeproj \
  -scheme "GRAINTests - ConsoleApp" -configuration Debug build
./Builds/MacOSX-Tests/build/Debug/GRAINTests

# Returns exit code 0 on success, 1 on failure (CI-friendly)
```

### Listening Tests Checklist

| Test | Pass criteria |
|------|---------------|
| Transparency (10-20% wet) | No obvious distortion, transients/tone unchanged |
| Bypass reveal | Reduced density when bypassed = effect working |
| Stability | No wobble/flutter on sustained pads |
| Level | No jump on bypass toggle |
| Smoothing | No clicks when automating Drive 0→100% |

## Branch Naming

Use task file numbers for branch names, not YouTrack ticket IDs:

```
feature/<task-number>-<short-description>
```

**Examples:**
- `feature/003-rms-detector` (from `tasks/003_rms_detector.md`)
- `feature/004-dynamic-bias` (from `tasks/004_dynamic_bias.md`)

## Code Style

This project uses clang-format and clang-tidy. Key conventions:

- **Indentation:** 4 spaces, no tabs
- **Braces:** Allman style (opening brace on new line)
- **Line length:** 120 characters max
- **Naming:** 
  - Classes: `CamelCase` (e.g., `RMSDetector`)
  - Methods/variables: `camelBack` (e.g., `processBlock`, `rmsLevel`)
  - Constants: `kCamelCase` or `UPPER_CASE` for macros
- **Pointers:** Left-aligned (`float* ptr`, not `float *ptr`)

Run `./scripts/format.sh fix` before committing.

### Development Workflow

```bash
# 1. Make changes

# 2. Build VST3
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 3. Run unit tests
./scripts/run_tests.sh

# 4. Run pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 5. If pass → test in DAW
# 6. If pass → commit
```

> **Note:** After modifying `.jucer` files, run Projucer `--resave` to regenerate Xcode projects.
> **Important:** Resave GRAINTests.jucer FIRST, then GRAIN.jucer — they share the `JuceLibraryCode/` directory,
> and GRAIN.jucer has the full module set that must win.
> ```bash
> /Users/sbrocos/JUCE/Projucer.app/Contents/MacOS/Projucer --resave GRAINTests.jucer
> /Users/sbrocos/JUCE/Projucer.app/Contents/MacOS/Projucer --resave GRAIN.jucer
> ```

## Build Commands

```bash
# Build VST3
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# Build Standalone
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" -configuration Debug build

# Build & Run Tests (shortcut)
./scripts/run_tests.sh
```

## Validation

```bash
# Use app bundle path (symlink in /usr/local/bin may not work)
/Applications/pluginval.app/Contents/MacOS/pluginval --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

## Current Status

- 83 tests passing (47 unit + 4 pipeline + 5 oversampling + 3 calibration + 5 standalone + 14 file player/transport + 5 transport bar UI)
- VST3 + Standalone build clean
- pluginval SUCCESS
- Internal oversampling: 2× real-time, 4× offline bounce
- Separate test target (GRAINTests.jucer) with CI-friendly exit codes

## Task Files

| Task | Description | Status |
|------|-------------|--------|
| `tasks/001_setup_testing.md` | Testing infrastructure | Done |
| `tasks/002_mvp_tanh_waveshaper.md` | MVP DSP implementation | Done |
| `tasks/003_rms_detector.md` | RMS envelope detector | Done |
| `tasks/004_dynamic_bias.md` | Dynamic bias for even harmonics | Done |
| `tasks/005_warmth_control.md` | Warmth parameter | Done |
| `tasks/006_spectral_focus.md` | Spectral focus (shelf EQ) | Done |
| `tasks/006b_architecture_refactor.md` | Architecture refactor & test target | Done |
| `tasks/006c_spectral_focus_reimpl.md` | Spectral focus mono module reimplementation | Done |
| `tasks/007_oversampling.md` | Internal oversampling (2×/4×) | Done |
| `tasks/008_plugin_editor.md` | Plugin editor Phase A + inputGain parameter | Done |

## Documentation

See `docs/` folder:
- `DELIVERABLE_EN.md` — Full PRD and academic deliverable
- `DEVELOPMENT_ENVIRONMENT.md` — Setup instructions
- `GRAIN_Code_Architecture.md` — Code architecture, class diagrams, signal flow
- `TESTING.md` — Testing strategy
- `Grain — Dsp Pipeline (diagram).pdf` — Visual signal flow diagram
