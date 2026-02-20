# GRAIN v1.0.0 — Validation Results

## Date: 2026-02-20

---

## Automated Tests

### Unit Tests (GRAINTests)

- Total: 95 tests
- Passed: 95
- Failed: 0
- Notes: All suites pass

| Test Suite | Tests | Result |
|------------|-------|--------|
| Waveshaper | 4 | PASS |
| Mix | 3 | PASS |
| Gain | 3 | PASS |
| Bypass behavior | 2 | PASS |
| Buffer processing | 2 | PASS |
| Parameter change | 1 | PASS |
| RMS Detector | 7 | PASS |
| Dynamic Bias | 7 | PASS |
| DC Blocker | 3 | PASS |
| DC Offset (pipeline) | 1 | PASS |
| Warmth | 7 | PASS |
| Focus (Spectral) | 7 | PASS |
| Standalone | 5 | PASS |
| Pipeline Integration | 4 | PASS |
| Oversampling | 5 | PASS |
| Calibration Config | 3 | PASS |
| File Player | 8 | PASS |
| Transport | 6 | PASS |
| Transport Bar UI | 5 | PASS |
| Waveform Display | 4 | PASS |
| Drag & Drop | 3 | PASS |
| Recorder | 5 | PASS |

### auval (AU Validation)

- Command: `auval -v aufx Grn1 BrWv`
- Result: **PASS**
- Notes: All AU validation checks passed

### pluginval (VST3 Validation)

- Strictness level: 10 (maximum)
- Result: **PASS**
- Tested configurations:
  - Sample rates: 44100, 48000, 96000 Hz
  - Block sizes: 64, 128, 256, 512, 1024
- Tests passed:
  - Plugin scan, open (cold/warm), info, programs
  - Audio processing, non-releasing audio processing
  - Plugin state, state restoration
  - Automation, automatable parameters
  - Parameter thread safety
  - Bus configuration (mono, stereo)
  - Fuzz parameters

### Release Build

- AU: **BUILD SUCCEEDED** (Release)
- VST3: **BUILD SUCCEEDED** (Release)
- Standalone: **BUILD SUCCEEDED** (Release)

---

## DAW Compatibility

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

---

## Listening Tests

### Transparency (10-20% wet)

- [ ] No perceived distortion
- [ ] No changes to transients
- [ ] No changes to tonal balance
- [ ] No changes to stereo image

### Bypass Reveal

- [ ] Plugin ON: signal has density/cohesion
- [ ] Plugin OFF: signal feels "less glued"
- [ ] Difference is subtle but noticeable when toggled

### Stability

| Material | Test | Result |
|----------|------|--------|
| Sustained pad (30+ sec) | No wobble | ☐ |
| Full mix (loud sections) | No pumping | ☐ |
| Silence | No noise/artifacts | ☐ |
| Transient-heavy drums | No softening | ☐ |

### Level Consistency

- [ ] Bypass toggle: no audible level jump
- [ ] Mix 0% to 100%: smooth transition
- [ ] Output gain: predictable behavior

---

## Performance

| Metric | Target | Result |
|--------|--------|--------|
| CPU (single instance) | < 5% | ☐ |
| CPU (8 instances) | < 30% | ☐ |
| Memory | Reasonable | ☐ |
| Latency (reported) | 0 samples | 0 samples |

---

## Edge Cases

- [ ] Sample rate 44.1 kHz: works
- [ ] Sample rate 48 kHz: works
- [ ] Sample rate 96 kHz: works
- [ ] Block size 64: no glitches
- [ ] Block size 2048: no glitches
- [ ] Mono input: handles correctly
- [ ] Extreme parameter values: no crash/NaN

*Note: pluginval tested 44.1/48/96 kHz at block sizes 64-1024 automatically — all passed.*

---

## Standalone App

- [ ] Launches without crash
- [ ] Audio device selector works
- [ ] Input/output device selection works
- [ ] Audio passes through with processing
- [ ] Meters respond correctly
- [ ] Bypass works
- [ ] All parameters functional
- [ ] Microphone permission requested (macOS)

---

## Known Issues

None identified during automated validation.

---

## Conclusion

GRAIN v1.0.0 passes all automated validation (95 unit tests, auval, pluginval strictness 10) and builds successfully in Release configuration for AU, VST3, and Standalone.

Manual validation (DAW compatibility, listening tests, performance) pending.
