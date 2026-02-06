# Task 004: Dynamic Bias (Level-Dependent Asymmetry)

## Objective

Implement a dynamic bias stage that introduces mild asymmetry based on the RMS level from Task 003. This promotes even-order harmonic generation (triode-like character) in a controlled, musical way. A DC blocker is included to prevent DC offset accumulation caused by the quadratic bias term.

```
Input -> [RMS Detector] -> envelope
              |
Input -> [Dynamic Bias] -> biased signal -> (to Waveshaper) -> ... -> [DC Blocker] -> Output
```

---

## Prerequisites

- Task 003 completed (RMS Detector working)

---

## Key Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| DC blocker | Included in this task | One-pole HPF ~5Hz prevents DC offset accumulation |
| RMS structure | Keep existing mono-sum | Use `currentEnvelope` directly from Task 003 |
| Bias parameter | Internal constant (`kBiasAmount = 0.3f`) | Not user-exposed; future: derived from "Grain" knob |
| DC offset test | Sine cycles + DC blocker, verify mean ~ 0 | Ensures pipeline correctness |
| Drive x Bias interaction | Documented as design consideration | Non-blocking |
| ParameterID version | 1 | Consistent with existing params |

---

## Design Rationale

### Why Dynamic Bias?

| Symmetric Waveshaper (tanh alone) | With Dynamic Bias |
|-----------------------------------|-------------------|
| Only odd harmonics (3rd, 5th, 7th...) | Even + odd harmonics (2nd, 3rd, 4th...) |
| "Cold", "digital" character | "Warm", "analog" character |
| Consistent regardless of level | More harmonics at higher levels |

### Triode-Like Behavior

Real tube amplifiers exhibit asymmetric transfer functions -- they clip positive and negative differently. This creates even harmonics (2nd, 4th, 6th) which our ears perceive as "warmth".

GRAIN simulates this subtly by adding a quadratic term proportional to the signal level before the waveshaper.

### Why Level-Dependent?

- **Low level signals:** Minimal bias -> mostly clean
- **Higher level signals:** More bias -> more even harmonics -> more "glue"

This creates a natural, musical response where the effect intensifies with signal energy.

---

## Architecture

### Constants (in `Source/DSP/GrainDSP.h`)

```cpp
constexpr float kBiasAmount = 0.3f;         // Internal bias intensity (future: from Grain param)
constexpr float kBiasScale = 0.1f;          // Keeps effect in micro-saturation territory
constexpr float kDCBlockerCutoffHz = 5.0f;  // DC blocker cutoff frequency
```

### Pure Function: `applyDynamicBias`

```cpp
inline float applyDynamicBias(float input, float rmsLevel, float biasAmount)
{
    float bias = rmsLevel * biasAmount * kBiasScale;
    return input + bias * input * input;
}
```

The quadratic term `bias * input * input` introduces asymmetry: for positive input the term is positive, for negative input the term is also positive (since `(-x)^2 = x^2`). This shifts both half-cycles upward, breaking symmetry.

### DC Blocker Struct

One-pole high-pass filter at ~5Hz to remove DC offset introduced by the quadratic bias:

```cpp
struct DCBlocker
{
    float x1 = 0.0f;
    float y1 = 0.0f;
    float R = 0.9993f;  // Default coefficient

    void prepare(float sampleRate)
    {
        constexpr float kTwoPi = 6.283185307f;
        R = 1.0f - (kTwoPi * kDCBlockerCutoffHz / sampleRate);
    }

    void reset() { x1 = 0.0f; y1 = 0.0f; }

    float process(float input)
    {
        float output = input - x1 + R * y1;
        x1 = input;
        y1 = output;
        return output;
    }
};
```

Note: Uses a local `kTwoPi` constant to keep `GrainDSP.h` dependency-free (no JUCE headers required).

---

## Signal Flow After This Task

```
Input -> RMS Detector (mono sum) -> currentEnvelope
  |                                    |
  +---> Dynamic Bias (quadratic) <-----+
            |
       Waveshaper (tanh)
            |
         Mix (dry/wet)
            |
       DC Blocker (HPF ~5Hz)    <- NEW
            |
         Gain (output)
            |
          Output
```

---

## Integration in processBlock

No new parameters or SmoothedValues needed (bias is a constant).

```cpp
// In prepareToPlay():
dcBlockerLeft.prepare(static_cast<float>(sampleRate));
dcBlockerLeft.reset();
dcBlockerRight.prepare(static_cast<float>(sampleRate));
dcBlockerRight.reset();

// In processBlock(), per-sample processing:
// Left channel
const float dry = leftSample;
const float biased = GrainDSP::applyDynamicBias(dry, currentEnvelope, GrainDSP::kBiasAmount);
const float wet = GrainDSP::applyWaveshaper(biased, drive);
const float mixed = GrainDSP::applyMix(dry, wet, mix);
const float dcBlocked = dcBlockerLeft.process(mixed);
leftChannel[sample] = GrainDSP::applyGain(dcBlocked, gain);

// Same pattern for right channel with dcBlockerRight
```

---

## Files to Modify

| File | Action |
|------|--------|
| `Source/DSP/GrainDSP.h` | Add constants, `applyDynamicBias`, `DCBlocker` struct |
| `Source/PluginProcessor.h` | Add two `DCBlocker` instances (left/right) |
| `Source/PluginProcessor.cpp` | Initialize DC blockers, integrate bias + DC blocker in pipeline |
| `Source/Tests/DSPTests.cpp` | Add dynamic bias, DC blocker, and DC offset accumulation tests |

---

## Unit Tests

### `runDynamicBiasTests()`

| # | Test | Assertion |
|---|------|-----------|
| 1 | Zero RMS produces no bias | `applyDynamicBias(0.5, 0.0, 1.0)` == 0.5 |
| 2 | Zero amount produces no bias | `applyDynamicBias(0.5, 0.5, 0.0)` == 0.5 |
| 3 | Positive input biased upward | result > input |
| 4 | Negative input biased upward (asymmetry) | result > input (quadratic always positive) |
| 5 | Asymmetric response | `|f(+0.5)| != |f(-0.5)|` |
| 6 | Higher RMS = more bias | deviation at RMS 0.9 > deviation at RMS 0.1 |
| 7 | Effect is subtle (bounded) | result within `input * (1 +/- kBiasScale)` bounds |

### `runDCBlockerTests()`

| # | Test | Assertion |
|---|------|-----------|
| 1 | Passes AC signal | Sine wave through DC blocker ~ original (after settling) |
| 2 | Removes DC offset | Constant DC input converges to 0 |
| 3 | Reset clears state | After reset, x1 and y1 are 0 |

### `runDCOffsetAccumulationTest()`

| # | Test | Assertion |
|---|------|-----------|
| 1 | Bias + DC blocker pipeline | Process ~10 cycles of 440Hz sine with bias (rms=0.5, biasAmount=1.0), then through DC blocker. Mean of output ~ 0 (within 0.01f) |

---

## Design Consideration: Drive x Bias Interaction

At high drive values (4x gain), the tanh is already in deep saturation, making the bias contribution less perceptible. At low drive (1x), the bias effect is more audible proportionally. This is acceptable behavior -- it creates a natural interaction where the bias "warmth" is most noticeable at moderate drive settings, which is the typical use case.

---

## BIAS_SCALE Constant

The `0.1f` scale factor ensures the effect stays subtle:

| biasAmount | rmsLevel | Effective bias |
|------------|----------|----------------|
| 1.0 | 1.0 | 0.1 (10% of input^2) |
| 0.5 | 0.5 | 0.025 (2.5% of input^2) |
| 0.3 | 0.3 | 0.009 (0.9% of input^2) |

This keeps GRAIN in "micro-saturation" territory per the PRD.

---

## Acceptance Criteria

### Functional
- [ ] Dynamic bias compiles without errors
- [ ] Bias with zero RMS produces unbiased signal
- [ ] Higher RMS produces more bias effect
- [ ] Effect is subtle even at maximum settings
- [ ] Asymmetry is measurable (positive/negative inputs treated differently)

### Quality
- [ ] pluginval still passes
- [ ] All previous tests still pass (15 from Task 001-003)
- [ ] New dynamic bias tests pass (7 tests)
- [ ] New DC blocker tests pass (3 tests)
- [ ] DC offset accumulation test passes (1 test)
- [ ] No DC offset accumulation over time

### Listening
- [ ] With bias active, subtle warmth compared to Task 002
- [ ] No pumping or instability (thanks to slow RMS)
- [ ] No low-frequency rumble from DC blocker
- [ ] Level-matched bypass toggle (no jump)

---

## Testing Checklist

```bash
# 1. Build VST3
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 2. Build & run Standalone (tests execute on load)
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" -configuration Debug build
./Builds/MacOSX/build/Debug/GRAIN.app/Contents/MacOS/GRAIN

# 3. Run pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 4. Verify all previous 15 tests + new 11 tests pass

# 5. Run format check
./scripts/format.sh fix
```

---

## Out of Scope (future tasks)

- Warmth control (Task 005) -- further harmonic shaping
- Focus control (Task 006) -- spectral emphasis
- Exposing bias as user parameter (future "Grain" knob)
