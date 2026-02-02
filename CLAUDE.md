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
│   └── DSP_ARCHITECTURE.md
├── tasks/                   # Implementation tasks (numbered)
├── Source/
│   ├── DSP/
│   │   └── GrainDSP.h       # Pure DSP functions (Task 002)
│   ├── Tests/
│   │   └── DSPTests.cpp     # Unit tests
│   ├── PluginProcessor.h    # Audio processing logic
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h       # GUI
│   └── PluginEditor.cpp
├── GRAIN.jucer              # Projucer project file
└── CLAUDE.md                # This file
```

## DSP Architecture

### Full Pipeline (future)
```
Input (Drive) → RMS Detector → Dynamic Bias → tanh Waveshaper → Warmth → Focus → Output → Mix
```

### MVP Pipeline (current)
```
Input → Drive (1x-4x) → tanh() → Mix (dry/wet) → Output (dB)
```

### Pure DSP Functions

Located in `Source/DSP/GrainDSP.h`:

```cpp
namespace GrainDSP
{
    float applyWaveshaper(float input, float drive);  // tanh saturation
    float applyMix(float dry, float wet, float mix);  // dry/wet blend
    float applyGain(float input, float gainLinear);   // output gain
}
```

**Bypass:** Implemented via mix smoothing (bypass ON → mix = 0). No separate bypass function.

## MVP Parameters

| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `drive` | Drive | 0.0–1.0 | 0.5 | Maps to 1x–4x gain |
| `mix` | Mix | 0.0–1.0 | 0.2 | 0 = dry, 1 = wet |
| `output` | Output | -12 to +12 dB | 0.0 | Final level trim |
| `bypass` | Bypass | 0/1 | 0 | Via mix smoothing |

## Design Principles

1. **Transparency:** No perceptible changes at 10–20% wet
2. **Stability:** No artifacts, no wobble on sustained material
3. **Safety:** No auto-gain, no hidden behavior
4. **Simplicity:** Minimal UI, no technical decisions for user

## Testing Philosophy

| What | How |
|------|-----|
| DSP logic (tanh, mix, gain) | Unit tests (pure functions) |
| Parameter smoothing | Discontinuity test + listening test |
| Plugin stability | pluginval |

### Test Constants
```cpp
namespace TestConstants
{
    constexpr float TOLERANCE = 1e-5f;
    constexpr float CLICK_THRESHOLD = 0.01f;  // -40dB
    constexpr int BUFFER_SIZE = 512;
}
```

### Running Tests
Tests run automatically when plugin loads in Debug mode:
```bash
# Build and run Standalone (tests execute on load)
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" -configuration Debug build

./Builds/MacOSX/build/Debug/GRAIN.app/Contents/MacOS/GRAIN
```

## Build Commands

```bash
# Build VST3
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" \
  -configuration Debug build

# Build Standalone
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" \
  -configuration Debug build
```

## Validation

```bash
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

## Current Status

- **Task 001:** Testing infrastructure (completed)
  - pluginval setup
  - Unit test scaffold with pure DSP functions
  - 15 tests passing (Waveshaper, Mix, Gain, Bypass, Buffer, Discontinuity)

- **Task 002:** MVP tanh waveshaper (next)
  - Extract GrainDSP.h
  - Implement APVTS parameters
  - Per-sample smoothing in processBlock

## Task Files

| Task | Description | Status |
|------|-------------|--------|
| `tasks/001_setup_testing.md` | Testing infrastructure | Done |
| `tasks/002_mvp_tanh_waveshaper.md` | MVP DSP implementation | Next |

## Documentation

See `docs/` folder:
- `DELIVERABLE_EN.md` — Full PRD and academic deliverable
- `DEVELOPMENT_ENVIRONMENT.md` — Setup instructions
- `DSP_ARCHITECTURE.md` — Signal flow diagrams
- `TESTING.md` — Testing strategy
