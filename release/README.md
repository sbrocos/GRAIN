# GRAIN — Micro-harmonic Saturation Processor

## Overview

GRAIN is a subtle saturation plugin designed for bus and mix processing. It adds density and cohesion without obvious distortion, preserving transients, tonal balance, and stereo image.

## Installation

### VST3 Plugin

Copy `GRAIN.vst3` to:
- `/Library/Audio/Plug-Ins/VST3/` (system-wide)
- `~/Library/Audio/Plug-Ins/VST3/` (user only)

### AU Plugin (Audio Unit)

Copy `GRAIN.component` to:
- `/Library/Audio/Plug-Ins/Components/` (system-wide)
- `~/Library/Audio/Plug-Ins/Components/` (user only)

### Standalone App

Copy `GRAIN.app` to `/Applications/` or run from any location.

## Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Input Gain | -12 to +12 dB | Pre-saturation level trim |
| Grain | 0-100% | Saturation intensity |
| Warmth | 0-100% | Even/odd harmonic balance |
| Focus | Low/Mid/High | Spectral emphasis |
| Mix | 0-100% | Dry/wet blend |
| Output | -12 to +12 dB | Output level trim |
| Bypass | On/Off | Bypass processing |

## Typical Usage

- **Mix bus:** 10-20% wet, default settings
- **Drum bus:** 20-40% wet, Focus on Low or Mid
- **Vocals:** 5-15% wet, Focus on Mid
- **Master bus:** 5-10% wet, subtle density

## Building from Source

### Prerequisites

- macOS 11.0+ (Apple Silicon)
- Xcode 16+
- JUCE 8.x (Projucer)

### Build

```bash
./bin/build -a            # Resave + build all targets + pluginval + open
./bin/run_tests           # Run unit tests + auval
```

### Release Build

```bash
./bin/build -R -u -v -s   # Release build: AU + VST3 + Standalone
```

## System Requirements

- macOS 11.0+ (Apple Silicon / ARM64)
- VST3 or AU compatible DAW

## License

GPL-3.0 — See LICENSE file
