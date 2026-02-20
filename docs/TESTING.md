# Testing Strategy — GRAIN

## Overview

GRAIN uses a pragmatic testing approach combining automated validation, unit tests for DSP logic, and manual listening tests.

---

## 1. Automated Validation: pluginval

**pluginval** is the industry-standard tool for validating audio plugins. It catches crashes, memory leaks, parameter issues, and VST3 compliance problems.

### Installation

Download from [GitHub](https://github.com/Tracktion/pluginval) or use the app bundle at `/Applications/pluginval.app`.

> **Note:** `brew install pluginval` may not work reliably. Use the app bundle path instead.

### Usage

```bash
# Use app bundle path (symlink in /usr/local/bin may not work)
PLUGINVAL=/Applications/pluginval.app/Contents/MacOS/pluginval

# Basic validation (during development — skip GUI tests for CI)
$PLUGINVAL --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Strict validation (recommended before release)
$PLUGINVAL --skip-gui-tests --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Verbose output
$PLUGINVAL --skip-gui-tests --verbose --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

### When to Run

- After every build during development
- Before every commit (recommended)
- Before any release

### What It Checks

- Plugin loads/unloads without crash
- Parameters report valid ranges
- No memory leaks
- State save/restore works
- Audio processing doesn't produce NaN/Inf
- Thread safety basics

---

## 2. Unit Tests: DSP Functions

Unit tests verify that individual DSP functions behave correctly in isolation.

### Framework

JUCE includes `juce::UnitTestRunner`. Tests live in `Source/Tests/`.

### Test Cases for MVP

#### Waveshaper Tests

| Test | Input | Expected Output |
|------|-------|-----------------|
| Zero passthrough | `tanh(0)` | `0` |
| Symmetry | `tanh(-x)` | `-tanh(x)` |
| Bounded output | `tanh(±∞)` | `±1` |
| Near-linear for small values | `tanh(0.1)` | `≈ 0.1` |

#### Mix Tests

| Test | Condition | Expected |
|------|-----------|----------|
| Full dry | mix = 0% | output = input |
| Full wet | mix = 100% | output = processed |
| 50/50 blend | mix = 50% | output = 0.5 * dry + 0.5 * wet |

#### Gain Tests

| Test | Condition | Expected |
|------|-----------|----------|
| Unity gain | output = 1.0 | output = input |
| Double gain | output = 2.0 | output = input * 2 |
| Zero gain | output = 0.0 | output = 0 (silence) |

### Running Tests

Tests run via a separate console application target (`GRAINTests.jucer`), not the plugin target.

```bash
# Quick: build + run via script
./bin/run_tests

# Manual: build test runner, then execute
xcodebuild -project Builds/MacOSX-Tests/GRAINTests.xcodeproj \
  -scheme "GRAINTests - ConsoleApp" -configuration Debug build
./Builds/MacOSX-Tests/build/Debug/GRAINTests

# Returns exit code 0 on success, 1 on failure (CI-friendly)
```

---

## 3. Listening Tests

Manual validation by ear. These cannot be automated but are essential for a saturation plugin.

### Test Material

Use consistent reference tracks:
- Drum bus (transient response)
- Pad/synth (sustained material, stability)
- Full mix (stereo image, overall color)

### MVP Listening Checklist

#### Transparency Test (10-20% wet)
- [ ] No obvious distortion
- [ ] Transients unchanged
- [ ] Tonal balance unchanged
- [ ] Stereo image unchanged

#### Bypass Reveal Test
- [ ] Enable plugin at 15% wet
- [ ] Listen for 30 seconds
- [ ] Bypass plugin
- [ ] Perception: reduced density/cohesion when bypassed = PASS

#### Stability Test
- [ ] Sustained pad through plugin
- [ ] No wobble, flutter, or pumping
- [ ] No artifacts on note changes

#### Level Test
- [ ] Bypass on/off: no noticeable level jump
- [ ] Output at 0dB = unity when Mix = 0%

#### Parameter Smoothing Test
- [ ] Automate Drive 0→100% quickly
- [ ] No clicks, pops, or zipper noise

---

## 4. Integration & Oversampling Tests

Already implemented in the test suite:

| File | Tests | What it verifies |
|------|-------|-----------------|
| `PipelineTest.cpp` | 4 | Full DSP chain: silence→silence, mix=0 dry, no NaN/Inf, level match |
| `OversamplingTest.cpp` | 5 | Silence passthrough, 2x/4x block sizes, latency bounds, signal integrity |
| `CalibrationTest.cpp` | 3 | Default config matches constants, extreme values safe, configs differ |

### Future additions
- Loading in headless host
- Automated A/B comparison
- CPU performance benchmarks

---

## Test Workflow

### During Development

```bash
# 1. Make changes

# 2. Build VST3
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 3. Run unit tests
./bin/run_tests

# 4. Run pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 5. If pass → test in DAW
# 6. If pass → commit
```

### Before Release

```bash
# Full validation suite
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Run unit tests
./bin/run_tests

# Perform full listening test checklist
# Test in multiple DAWs (Logic, Ableton)
```

---

## Folder Structure

```
GRAIN/
├── Source/
│   ├── DSP/                     # DSP modules under test
│   │   └── *.h                  # Header-only modules
│   └── Tests/
│       ├── TestMain.cpp         # Console app entry point
│       ├── DSPTests.cpp         # Unit tests (47 tests: waveshaper, mix, gain, RMS, bias, DC, warmth, focus)
│       ├── PipelineTest.cpp     # Integration tests (4 tests: full pipeline)
│       ├── OversamplingTest.cpp # Oversampling tests (5 tests)
│       └── CalibrationTest.cpp  # CalibrationConfig tests (3 tests)
│       ├── FilePlayerTest.cpp   # File player/transport tests (14 tests)
│       ├── TransportBarTest.cpp # Transport bar UI tests (5 tests)
│       ├── WaveformTest.cpp     # Waveform display tests (4 tests)
│       ├── DragDropTest.cpp     # Drag & drop tests (3 tests)
│       └── RecorderTest.cpp     # Recorder tests (5 tests)
├── GRAINTests.jucer             # Separate Projucer project for test runner
├── bin/
│   └── run_tests               # Build & run all tests + auval
└── docs/
    └── TESTING.md               # This file
```

**Current count:** 95 tests (47 unit + 4 pipeline + 5 oversampling + 3 calibration + 5 standalone + 14 file player/transport + 5 transport bar UI + 4 waveform display + 3 drag & drop + 5 recorder)

---

## Acceptance Criteria Summary

| Check | Tool | Required for MVP |
|-------|------|------------------|
| Plugin loads without crash | pluginval | ✅ |
| No memory leaks | pluginval | ✅ |
| Parameters valid | pluginval | ✅ |
| Waveshaper math correct | Unit test | ✅ |
| Mix blends correctly | Unit test | ✅ |
| Gain scales correctly | Unit test | ✅ |
| Sounds transparent at 15% | Listening | ✅ |
| Bypass reveals effect | Listening | ✅ |
| No artifacts on sustained | Listening | ✅ |
| No zipper noise | Listening | ✅ |
