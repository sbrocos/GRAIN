# Testing Strategy — GRAIN

## Overview

GRAIN uses a pragmatic testing approach combining automated validation, unit tests for DSP logic, and manual listening tests.

---

## 1. Automated Validation: pluginval

**pluginval** is the industry-standard tool for validating audio plugins. It catches crashes, memory leaks, parameter issues, and VST3 compliance problems.

### Installation

```bash
brew install pluginval
```

### Usage

```bash
# Basic validation
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Strict validation (recommended before release)
pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Verbose output
pluginval --verbose --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
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

```bash
# Build and run test target
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" \
  -configuration Debug build

# Run tests (from standalone app console or dedicated test runner)
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

## 4. Integration Tests (Future)

Not required for MVP. Would include:
- Loading in headless host
- Automated A/B comparison
- CPU performance benchmarks

---

## Test Workflow

### During Development

```bash
# 1. Make changes
# 2. Build
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 3. Run pluginval
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 4. If pass, test in DAW
# 5. If pass, commit
```

### Before Release

```bash
# Full validation suite
pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Run unit tests
# Perform full listening test checklist
# Test in multiple DAWs (Logic, Ableton)
```

---

## Folder Structure

```
GRAIN/
├── Source/
│   ├── PluginProcessor.cpp
│   ├── PluginProcessor.h
│   └── Tests/
│       └── DSPTests.cpp      # Unit tests
└── docs/
    └── TESTING.md            # This file
```

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
