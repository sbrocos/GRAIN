# Task 007b: Centralized Calibration Config

## Objective

Replace scattered `constexpr` values in `Constants.h` with a centralized, hierarchical `CalibrationConfig.h` based on typed structs. Each DSP module receives its calibration via `prepare()` instead of reading global constants directly. This improves iterability during the listening calibration phase without adding runtime overhead or external file dependencies.

## Prerequisites

- Task 006b completed (modules split into separate files, `DSPPipeline` exists)
- Task 007 completed (oversampling integrated)
- All tests pass
- pluginval passes

---

## Current State

After 006b, DSP calibration values live in `Source/DSP/Constants.h` as flat `constexpr` values:

```cpp
namespace GrainDSP
{
constexpr float kTwoPi = 6.283185307f;
constexpr float kRmsAttackMs = 100.0f;
constexpr float kRmsReleaseMs = 300.0f;
constexpr float kBiasAmount = 0.3f;
constexpr float kBiasScale = 0.1f;
constexpr float kDCBlockerCutoffHz = 5.0f;
constexpr float kWarmthDepth = 0.22f;
constexpr float kFocusLowShelfFreq = 200.0f;
constexpr float kFocusHighShelfFreq = 4000.0f;
constexpr float kFocusShelfGainDb = 3.0f;
constexpr float kFocusShelfQ = 0.707f;
}
```

**Problems:**

1. **Flat namespace** — no grouping by module; hard to find which constant belongs where.
2. **Tight coupling** — modules read globals directly, making it impossible to test with alternative calibrations.
3. **No iteration affordance** — changing a calibration value requires hunting through `Constants.h` and understanding which module uses it.
4. **Waveshaper drive mapping is implicit** — the `driveMin`/`driveMax` range (0.1–0.5) is buried inside the Waveshaper, not in constants at all.

---

## Target State

### New file: `Source/DSP/CalibrationConfig.h`

```cpp
#pragma once

namespace GrainDSP
{

struct RMSCalibration
{
    float attackMs = 100.0f;
    float releaseMs = 300.0f;
};

struct BiasCalibration
{
    float amount = 0.3f;
    float scale = 0.1f;
};

struct WaveshaperCalibration
{
    float driveMin = 0.1f;    // Grain 0% maps to this drive
    float driveMax = 0.5f;    // Grain 100% maps to this drive
};

struct WarmthCalibration
{
    float depth = 0.22f;
};

struct FocusCalibration
{
    float lowShelfFreq = 200.0f;
    float highShelfFreq = 4000.0f;
    float shelfGainDb = 3.0f;
    float shelfQ = 0.707f;
};

struct DCBlockerCalibration
{
    float cutoffHz = 5.0f;
};

struct CalibrationConfig
{
    RMSCalibration rms;
    BiasCalibration bias;
    WaveshaperCalibration waveshaper;
    WarmthCalibration warmth;
    FocusCalibration focus;
    DCBlockerCalibration dcBlocker;
};

// Default calibration — reference values for GRAIN's "safe" character
inline constexpr CalibrationConfig kDefaultCalibration{};

} // namespace GrainDSP
```

### Updated `Constants.h` (residual)

After migration, only non-calibration constants remain:

```cpp
#pragma once

namespace GrainDSP
{
constexpr float kTwoPi = 6.283185307f;
}
```

If `kTwoPi` is the only remaining constant, move it to `DSPHelpers.h` and delete `Constants.h`.

---

## Module API Changes

Each DSP module's `prepare()` gains a calibration parameter. The processing functions that currently read global constants receive the relevant values as arguments instead.

### RMSDetector.h

```cpp
// BEFORE
void prepare(float sampleRate)
{
    attackCoeff = calculateCoefficient(sampleRate, kRmsAttackMs);
    releaseCoeff = calculateCoefficient(sampleRate, kRmsReleaseMs);
}

// AFTER
void prepare(float sampleRate, const RMSCalibration& cal)
{
    attackCoeff = calculateCoefficient(sampleRate, cal.attackMs);
    releaseCoeff = calculateCoefficient(sampleRate, cal.releaseMs);
}
```

### DynamicBias.h

```cpp
// BEFORE
inline float applyDynamicBias(float sample, float rmsLevel, float biasAmount)
{
    float bias = rmsLevel * biasAmount * kBiasScale;
    return sample + bias * sample * sample;
}

// AFTER
inline float applyDynamicBias(float sample, float rmsLevel,
                               const BiasCalibration& cal)
{
    float bias = rmsLevel * cal.amount * cal.scale;
    return sample + bias * sample * sample;
}
```

**Note:** If `biasAmount` is currently passed as a parameter from the pipeline (not from constants), keep it as a parameter and only replace `kBiasScale` with `cal.scale`. Adapt to whatever the actual current signature is.

### Waveshaper.h

```cpp
// BEFORE (drive mapping hardcoded or implicit)
inline float mapGrainToDrive(float grainPercent)
{
    return juce::jmap(grainPercent, 0.0f, 100.0f, 0.1f, 0.5f);
}

// AFTER
inline float mapGrainToDrive(float grainPercent,
                              const WaveshaperCalibration& cal)
{
    return juce::jmap(grainPercent, 0.0f, 100.0f,
                      cal.driveMin, cal.driveMax);
}
```

### WarmthProcessor.h

```cpp
// BEFORE
inline float applyWarmth(float input, float warmth)
{
    float depth = warmth * kWarmthDepth;
    // ...
}

// AFTER
inline float applyWarmth(float input, float warmth,
                          const WarmthCalibration& cal)
{
    float depth = warmth * cal.depth;
    // ...
}
```

### SpectralFocus.h

```cpp
// BEFORE
void prepare(float sampleRate, FocusMode mode)
{
    calculateCoefficients(sampleRate, mode,
        kFocusLowShelfFreq, kFocusHighShelfFreq,
        kFocusShelfGainDb, kFocusShelfQ);
}

// AFTER
void prepare(float sampleRate, FocusMode mode,
             const FocusCalibration& cal)
{
    calculateCoefficients(sampleRate, mode,
        cal.lowShelfFreq, cal.highShelfFreq,
        cal.shelfGainDb, cal.shelfQ);
}
```

### DCBlocker.h

```cpp
// BEFORE
void prepare(float sampleRate)
{
    float rc = 1.0f / (kTwoPi * kDCBlockerCutoffHz);
    // ...
}

// AFTER
void prepare(float sampleRate, const DCBlockerCalibration& cal)
{
    constexpr float twoPi = 6.283185307f;
    float rc = 1.0f / (twoPi * cal.cutoffHz);
    // ...
}
```

### GrainDSPPipeline.h

```cpp
// BEFORE
void prepare(float sampleRate, FocusMode focusMode)
{
    dcBlocker.prepare(sampleRate);
    spectralFocus.prepare(sampleRate, focusMode);
    // ...
}

// AFTER
void prepare(float sampleRate, FocusMode focusMode,
             const CalibrationConfig& cal)
{
    config = cal;  // Store for use in processSample
    rmsDetector.prepare(sampleRate, cal.rms);
    dcBlocker.prepare(sampleRate, cal.dcBlocker);
    spectralFocus.prepare(sampleRate, focusMode, cal.focus);
    // ...
}

float processSample(float dry, float envelope, float drive,
                    float warmth, float mix, float gain)
{
    const float biased = applyDynamicBias(dry, envelope, config.bias);
    const float shaped = applyWaveshaper(biased, drive);
    const float warmed = applyWarmth(shaped, warmth, config.warmth);
    // ...
}

private:
    CalibrationConfig config;
```

### PluginProcessor

```cpp
// Member
GrainDSP::CalibrationConfig calibration = GrainDSP::kDefaultCalibration;

// In prepareToPlay()
pipelineLeft.prepare(oversampledRate, focusMode, calibration);
pipelineRight.prepare(oversampledRate, focusMode, calibration);
```

---

## Execution Order

1. Create `Source/DSP/CalibrationConfig.h` with all structs and `kDefaultCalibration`
2. Update `GrainDSPPipeline` to accept and store `const CalibrationConfig&`
3. Update each DSP module's `prepare()` / processing functions (one at a time, compile between each)
4. Update `PluginProcessor` to pass calibration to pipelines
5. Migrate constants from `Constants.h` — remove each `constexpr` as it gets a home in a struct
6. Clean up `Constants.h` (move `kTwoPi` to `DSPHelpers.h` if it's the only remaining item, then delete)
7. Update all tests to use new API signatures (use `kDefaultCalibration` or sub-configs)
8. Add new calibration-specific tests
9. Verify clean compilation (`-Wall -Wextra`)
10. Run pluginval

---

## Tests

### Update existing tests

All existing tests that call `prepare()` or processing functions need to pass calibration. Use `kDefaultCalibration` to maintain identical behavior:

```cpp
// BEFORE
rmsDetector.prepare(44100.0f);

// AFTER
rmsDetector.prepare(44100.0f, GrainDSP::kDefaultCalibration.rms);
```

### New tests to add

#### 1. Default calibration regression test

Verify that `kDefaultCalibration` produces the same numerical values as the original constants. This is a safety net during migration — can be removed after verification.

```cpp
beginTest("Default calibration matches original constants");
{
    auto cal = GrainDSP::kDefaultCalibration;
    expectEquals(cal.rms.attackMs, 100.0f);
    expectEquals(cal.rms.releaseMs, 300.0f);
    expectEquals(cal.bias.amount, 0.3f);
    expectEquals(cal.bias.scale, 0.1f);
    expectEquals(cal.waveshaper.driveMin, 0.1f);
    expectEquals(cal.waveshaper.driveMax, 0.5f);
    expectEquals(cal.warmth.depth, 0.22f);
    expectEquals(cal.focus.lowShelfFreq, 200.0f);
    expectEquals(cal.focus.highShelfFreq, 4000.0f);
    expectEquals(cal.focus.shelfGainDb, 3.0f);
    expectEquals(cal.focus.shelfQ, 0.707f);
    expectEquals(cal.dcBlocker.cutoffHz, 5.0f);
}
```

#### 2. Custom calibration — extreme values don't produce NaN/Inf

```cpp
beginTest("Extreme calibration produces no NaN/Inf");
{
    GrainDSP::CalibrationConfig extreme;
    extreme.rms.attackMs = 1.0f;       // Very fast
    extreme.rms.releaseMs = 5000.0f;   // Very slow
    extreme.bias.amount = 1.0f;        // Max bias
    extreme.bias.scale = 1.0f;         // Max scale
    extreme.waveshaper.driveMin = 0.01f;
    extreme.waveshaper.driveMax = 2.0f; // Beyond safe zone
    extreme.warmth.depth = 1.0f;       // Max warmth
    extreme.focus.shelfGainDb = 12.0f; // Aggressive shelving

    GrainDSP::DSPPipeline pipeline;
    pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid, extreme);

    // Process 1000 samples of sine wave
    for (int i = 0; i < 1000; ++i)
    {
        float sample = std::sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
        float result = pipeline.processSample(sample, 0.5f, 1.0f,
                                               1.0f, 1.0f, 1.0f);
        expect(!std::isnan(result));
        expect(!std::isinf(result));
    }
}
```

#### 3. Custom calibration — different configs produce different output

```cpp
beginTest("Different calibrations produce different output");
{
    GrainDSP::DSPPipeline pipelineA;
    GrainDSP::DSPPipeline pipelineB;

    GrainDSP::CalibrationConfig configA = GrainDSP::kDefaultCalibration;
    GrainDSP::CalibrationConfig configB = GrainDSP::kDefaultCalibration;
    configB.warmth.depth = 0.8f;  // Much more warmth

    pipelineA.prepare(44100.0f, GrainDSP::FocusMode::Mid, configA);
    pipelineB.prepare(44100.0f, GrainDSP::FocusMode::Mid, configB);

    float sample = 0.5f;
    float resultA = pipelineA.processSample(sample, 0.5f, 0.3f,
                                             0.5f, 1.0f, 1.0f);
    float resultB = pipelineB.processSample(sample, 0.5f, 0.3f,
                                             0.5f, 1.0f, 1.0f);

    expect(std::abs(resultA - resultB) > 1e-6f);
}
```

---

## Constraints

1. **NO numerical value changes** — only reorganize where values live. Output must be bit-identical with `kDefaultCalibration`.
2. **NO file I/O** — purely compile-time. No YAML, JSON, or external files.
3. **NO breaking existing tests** — update signatures but preserve behavior.
4. **Header-only** — no .cpp files for the config.
5. **Allman brace style** per project convention.
6. **Stay in `GrainDSP` namespace.**

---

## Acceptance Criteria

### Functional
- [ ] `CalibrationConfig.h` exists with all sub-structs
- [ ] `kDefaultCalibration` compiles as `constexpr`
- [ ] Every DSP module receives its calibration via `prepare()` or function params
- [ ] No DSP module directly reads from `Constants.h` calibration values
- [ ] `Constants.h` is either deleted or contains only math constants
- [ ] `PluginProcessor` holds a `CalibrationConfig` member and passes it to pipelines

### Tests
- [ ] All existing tests updated and passing
- [ ] Default calibration regression test passes
- [ ] Extreme calibration NaN/Inf test passes
- [ ] Different calibration produces different output test passes
- [ ] pluginval still passes

### Code Quality
- [ ] No circular dependencies
- [ ] Clean compilation with `-Wall -Wextra`
- [ ] clang-format passes on all modified files

---

## Out of Scope

- Runtime config loading from files (JSON/YAML) — future enhancement if compile-test cycle becomes a bottleneck
- UI for calibration tweaking
- Per-preset calibration profiles
- Any DSP algorithm changes — this is purely structural

---

## Risk: Audio Regression

Same mitigation as 006b: process a known input (1s of 440Hz sine, specific parameter values) before and after. Output should be identical within float epsilon (~1e-7) when using `kDefaultCalibration`.
