# Task 010: Final Validation + Release

> **Nota:** Esta numeración es temporal. Se renumerará después de insertar la task de Custom UI (Phase B) que se definirá en otro chat.

## Objective

Complete final validation of GRAIN (VST3 + Standalone) and prepare the release package for the academic deliverable.

---

## Prerequisites

- All previous tasks completed (001-009)
- Plugin and Standalone fully functional
- UI implemented (at least Phase A)

---

## Validation Checklist

### 1. Automated Validation

#### pluginval (Maximum Strictness)

```bash
# Run with strictness level 10
pluginval --strictness-level 10 \
  --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# Expected: All tests pass
```

#### Unit Tests

```bash
# Build and run tests (Debug build triggers tests)
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# Check console output for test results
```

| Test Suite | Expected |
|------------|----------|
| Waveshaper tests | ✅ All pass |
| Mix tests | ✅ All pass |
| Gain tests | ✅ All pass |
| RMS Detector tests | ✅ All pass |
| Dynamic Bias tests | ✅ All pass |
| Warmth tests | ✅ All pass |
| Spectral Focus tests | ✅ All pass |
| Oversampling tests | ✅ All pass |
| Buffer stability tests | ✅ All pass |
| Discontinuity tests | ✅ All pass |

### 2. DAW Compatibility

Test in primary DAWs:

| DAW | Load | Process | Automate | Bypass | Save/Recall |
|-----|------|---------|----------|--------|-------------|
| Logic Pro | ☐ | ☐ | ☐ | ☐ | ☐ |
| Ableton Live | ☐ | ☐ | ☐ | ☐ | ☐ |

**Test procedure per DAW:**
1. Load plugin on stereo track
2. Play audio through it
3. Adjust all parameters
4. Automate at least one parameter (Grain or Mix)
5. Toggle bypass multiple times
6. Save project, close, reopen
7. Verify state restored correctly

### 3. Listening Validation

From the PRD validation plan:

#### At 10-20% Wet
- [ ] No perceived distortion
- [ ] No changes to transients
- [ ] No changes to tonal balance
- [ ] No changes to stereo image

#### Bypass Reveal Test
- [ ] Plugin ON: signal has density/cohesion
- [ ] Plugin OFF: signal feels "less glued"
- [ ] Difference is subtle but noticeable when toggled

#### Stability Tests
| Material | Test | Expected |
|----------|------|----------|
| Sustained pad (30+ sec) | No wobble | ✅ Stable |
| Full mix (loud sections) | No pumping | ✅ Stable |
| Silence | No noise/artifacts | ✅ Silent |
| Transient-heavy drums | No softening | ✅ Preserved |

#### Level Consistency
- [ ] Bypass toggle: no audible level jump
- [ ] Mix 0% → 100%: smooth transition
- [ ] Output gain: predictable behavior

### 4. Performance Check

| Metric | Target | Method |
|--------|--------|--------|
| CPU (single instance) | < 5% | DAW CPU meter |
| CPU (8 instances) | < 30% | DAW CPU meter |
| Memory | Reasonable | Activity Monitor |
| Latency (reported) | Correct | DAW latency display |

### 5. Edge Cases

- [ ] Sample rate 44.1 kHz: works
- [ ] Sample rate 48 kHz: works
- [ ] Sample rate 96 kHz: works
- [ ] Block size 64: no glitches
- [ ] Block size 2048: no glitches
- [ ] Mono input: handles correctly (or documented limitation)
- [ ] Extreme parameter values: no crash/NaN

### 6. Standalone App Validation

- [ ] Launches without crash
- [ ] Audio device selector works
- [ ] Input/output device selection works
- [ ] Audio passes through with processing
- [ ] Meters respond correctly
- [ ] Bypass works
- [ ] All parameters functional
- [ ] Microphone permission requested (macOS)

---

## Release Build

### Build Configuration

```bash
# Release build (optimized)
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" \
  -configuration Release \
  build

xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" \
  -configuration Release \
  build
```

### Code Signing (Optional for Academic)

For academic submission, unsigned builds are acceptable. For distribution:

```bash
# Sign the VST3 bundle
codesign --force --deep --sign "Developer ID Application: YOUR_NAME" \
  ./Builds/MacOSX/build/Release/GRAIN.vst3

# Sign the standalone app
codesign --force --deep --sign "Developer ID Application: YOUR_NAME" \
  ./Builds/MacOSX/build/Release/GRAIN.app
```

### Notarization (Optional)

For distribution outside the Mac App Store, notarization is required. Skip for academic submission.

---

## Release Package

### Directory Structure

```
GRAIN-v1.0/
├── Plugin/
│   └── GRAIN.vst3           # VST3 bundle
├── Standalone/
│   └── GRAIN.app            # Standalone application
├── Documentation/
│   ├── README.md            # User guide
│   ├── INSTALL.md           # Installation instructions
│   └── CHANGELOG.md         # Version history
├── Source/                   # (optional for academic)
│   └── ... (full source code)
└── LICENSE                   # GPL-3.0
```

### README.md Content

```markdown
# GRAIN — Micro-harmonic Saturation Processor

## Overview
GRAIN is a subtle saturation plugin designed for bus and mix processing.
It adds density and cohesion without obvious distortion.

## Installation

### VST3 Plugin
Copy `GRAIN.vst3` to:
- `/Library/Audio/Plug-Ins/VST3/` (system-wide)
- `~/Library/Audio/Plug-Ins/VST3/` (user only)

### Standalone App
Copy `GRAIN.app` to `/Applications/` or run from any location.

## Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Grain | 0-100% | Saturation intensity |
| Warmth | 0-100% | Even/odd harmonic balance |
| Focus | Low/Mid/High | Spectral emphasis |
| Mix | 0-100% | Dry/wet blend |
| Output | ±12 dB | Output level |

## Typical Usage
- Mix bus: 10-20% wet, default settings
- Drum bus: 20-40% wet, Focus on Low or Mid
- Vocals: 5-15% wet, Focus on Mid

## System Requirements
- macOS 11.0+ (Apple Silicon)
- VST3-compatible DAW

## License
GPL-3.0 — See LICENSE file
```

### INSTALL.md Content

```markdown
# Installation Guide

## VST3 Plugin

1. Locate the `GRAIN.vst3` file
2. Copy to your VST3 folder:
   - System: `/Library/Audio/Plug-Ins/VST3/`
   - User: `~/Library/Audio/Plug-Ins/VST3/`
3. Restart your DAW
4. Scan for new plugins if necessary

## Standalone Application

1. Locate the `GRAIN.app` file
2. Copy to `/Applications/` (optional)
3. Double-click to launch
4. Grant microphone permission when prompted
5. Select audio input/output devices

## Troubleshooting

### Plugin not appearing in DAW
- Verify the .vst3 file is in the correct folder
- Rescan plugins in your DAW
- Check DAW's plugin manager for blacklisted plugins

### Audio not passing through standalone
- Check audio device selection
- Verify microphone permissions in System Preferences
- Ensure input/output devices are working
```

---

## Academic Deliverable Checklist

From the PRD:

- [ ] Final PRD/PDR document (Markdown) — already in docs/
- [ ] Source code with documentation
- [ ] README with build steps
- [ ] Binaries (plugin + standalone)
- [ ] Validation results documented
- [ ] Listening test notes

### Documentation Files

| File | Location | Status |
|------|----------|--------|
| DELIVERABLE_EN.md | docs/ | ✅ Exists |
| DEVELOPMENT_ENVIRONMENT.md | docs/ | ✅ Exists |
| TESTING.md | docs/ | ✅ Exists |
| DSP_ARCHITECTURE.md | docs/ | ✅ Exists |
| CLAUDE.md | project root | ✅ Exists |
| README.md | release package | ☐ Create |
| INSTALL.md | release package | ☐ Create |
| VALIDATION_RESULTS.md | docs/ | ☐ Create after testing |

---

## Validation Results Template

Create `docs/VALIDATION_RESULTS.md` after testing:

```markdown
# GRAIN v1.0 — Validation Results

## Date: [DATE]

## Automated Tests

### pluginval
- Strictness level: 10
- Result: PASS / FAIL
- Notes: [any issues]

### Unit Tests
- Total: XX tests
- Passed: XX
- Failed: XX
- Notes: [any issues]

## DAW Testing

| DAW | Version | Result | Notes |
|-----|---------|--------|-------|
| Logic Pro | X.X | ✅ | — |
| Ableton Live | XX | ✅ | — |

## Listening Tests

### Transparency (10-20% wet)
- Distortion: Not audible ✅
- Transients: Preserved ✅
- Tonal balance: Unchanged ✅
- Stereo image: Unchanged ✅

### Bypass Reveal
- Effect perceived: Yes ✅
- Density/cohesion difference: Subtle but clear ✅

### Stability
- Sustained material: Stable ✅
- Level jumps on bypass: None ✅

## Performance

| Metric | Result |
|--------|--------|
| CPU (single) | X% |
| CPU (8 instances) | X% |
| Latency | X samples |

## Known Issues
- [List any known issues for V1.0]

## Conclusion
GRAIN v1.0 meets all acceptance criteria for the academic deliverable.
```

---

## Files to Create

| File | Action |
|------|--------|
| `release/README.md` | User documentation |
| `release/INSTALL.md` | Installation guide |
| `docs/VALIDATION_RESULTS.md` | Test results (after validation) |

---

## Acceptance Criteria

### Validation
- [ ] pluginval passes at strictness 10
- [ ] All unit tests pass
- [ ] Works in Logic Pro and Ableton Live
- [ ] Listening tests confirm transparency and stability
- [ ] Performance acceptable (< 5% CPU single instance)

### Release Package
- [ ] VST3 builds successfully (Release)
- [ ] Standalone builds successfully (Release)
- [ ] README included
- [ ] INSTALL instructions included
- [ ] Source code organized

### Academic
- [ ] PRD document complete
- [ ] All documentation in docs/
- [ ] Validation results documented
- [ ] Build reproducible from source

---

## Post-Release (V1.1 Roadmap)

From the PRD, planned for V1.1:
- A/B comparison feature
- Presets
- Optional stereo Link/Unlink
- Custom UI (Phase B)

These are out of scope for this task.
