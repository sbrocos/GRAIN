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
│   ├── PluginProcessor.h    # Audio processing logic
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h       # GUI
│   ├── PluginEditor.cpp
│   └── Tests/
│       └── DSPTests.cpp
├── GRAIN.jucer              # Projucer project file
└── CLAUDE.md                # This file
```

## DSP Architecture

```
Input (Drive) → RMS Detector → Dynamic Bias → tanh Waveshaper → Warmth → Focus → Output → Mix
```

**MVP (current focus):**
```
Input (Drive) → tanh → Mix → Output
```

## Key Parameters

| Parameter | Range | Purpose |
|-----------|-------|---------|
| Drive | 0–100% | Input gain (limited range) |
| Grain | 0–100% | Saturation intensity |
| Warmth | 0–100% | Even/odd harmonic balance (subtle) |
| Focus | Low/Mid/High | Spectral emphasis |
| Mix | 0–100% | Dry/wet blend |
| Output | -12 to +12 dB | Final gain trim |

## Design Principles

1. **Transparency:** No perceptible changes at 10–20% wet
2. **Stability:** No artifacts, no wobble on sustained material
3. **Safety:** No auto-gain, no hidden behavior
4. **Simplicity:** Minimal UI, no technical decisions for user

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

## Testing

- **DAWs:** Logic Pro, Ableton Live
- **VST3 path:** `~/Library/Audio/Plug-Ins/VST3/`

### Validation
```bash
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

### Unit Tests
Run automatically in Debug builds (see `Source/Tests/`)

### DAWs
Logic Pro, Ableton Live

### VST3 Path
`~/Library/Audio/Plug-Ins/VST3/`

## Current Task

See `tasks/` folder for numbered implementation tasks.

## Documentation

See `docs/` folder for detailed specifications:
- `DELIVERABLE_EN.md` — Full PRD and academic deliverable
- `DEVELOPMENT_ENVIRONMENT.md` — Setup instructions
- `DSP_ARCHITECTURE.md` — Signal flow diagrams
- `TESTING.md` — Testing strategy (pluginval, unit tests, listening tests)
