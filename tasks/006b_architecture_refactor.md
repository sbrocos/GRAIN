# Task 006b: Architecture Refactor & Code Alignment

## Objective

Refactor the codebase to align with the architecture documented in `docs/GRAIN_Code_Architecture.md`, fix technical debt accumulated during rapid prototyping (Tasks 001–006), and prepare the codebase for oversampling (Task 007).

## Prerequisites

- Task 006 completed (full DSP chain working)
- All existing tests pass
- pluginval passes

---

## Current State (Problems)

### P1 — GrainDSP.h is a 415-line monolith

Contains 7 distinct abstractions in a single header-only file:
- Constants (lines 1–43)
- `calculateCoefficient()` utility (lines 49–52)
- `RMSDetector` struct (lines 60–92)
- `applyDynamicBias()` function (lines 100–105)
- `applyWarmth()` function (lines 114–120)
- `SpectralFocus` struct with nested `BiquadState` (lines 125–285) — 161 lines
- `DCBlocker` struct (lines 292–325)
- `applyWaveshaper()`, `applyMix()`, `applyGain()` functions (lines 327–355)
- `processSample()` orchestrator function (lines 365–380)

The order doesn't follow the signal chain. Adding oversampling to this file would make it unmanageable.

### P2 — Gap between architecture doc and actual code

`docs/GRAIN_Code_Architecture.md` describes:
- Separate files per module: `RMSDetector.h`, `DynamicBias.h`, `Waveshaper.h`, etc.
- A `GrainDSPPipeline` class that orchestrates modules
- A `GrainParameters` class for centralized parameter management

None of this exists. The code diverged toward a pragmatic monolith.

### P3 — Bypass is `AudioParameterFloat` instead of `AudioParameterBool`

```cpp
// Current (wrong)
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID("bypass", 1), "Bypass",
    juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

// Shows as a slider in GenericEditor. Semantically incorrect.
```

### P4 — No integration test for `processSample()`

Each DSP stage is tested individually, but the full chain composition (`processSample()` in `GrainDSP.h`) is never verified. Edge cases like silence→silence, mix=0 passthrough, and NaN propagation through the chain are untested.

### P5 — Inconsistent stereo state pattern

Two different patterns coexist:

```cpp
// Pattern A: Separate instances per channel (DCBlocker)
GrainDSP::DCBlocker dcBlockerLeft;
GrainDSP::DCBlocker dcBlockerRight;

// Pattern B: Internal arrays (SpectralFocus)
BiquadState lowShelf[2];   // [0]=left, [1]=right
BiquadState highShelf[2];
```

### P6 — `#include "Tests/DSPTests.cpp"` is a hack

```cpp
#include "Tests/DSPTests.cpp"  // NOLINT(bugprone-suspicious-include)
```

Tests compile inside the production plugin binary (Debug). A `static` global `TestRunner` auto-runs tests on plugin load. This means:
- Production binary is larger than needed
- Tests can't run independently
- pluginval validates a binary that contains test code
- Test failures could affect plugin behavior

---

## Target State

### File Structure After Refactor

```
Source/
├── DSP/
│   ├── Constants.h              // All DSP constants
│   ├── DSPHelpers.h             // Utility functions (calculateCoefficient, applyMix, applyGain)
│   ├── RMSDetector.h            // Slow envelope follower
│   ├── DynamicBias.h            // Level-dependent asymmetry
│   ├── Waveshaper.h             // tanh saturation
│   ├── WarmthProcessor.h        // Even/odd harmonic balance
│   ├── SpectralFocus.h          // Low/Mid/High spectral bias (mono, no JUCE dependency)
│   ├── DCBlocker.h              // DC offset removal
│   └── GrainDSPPipeline.h      // Orchestrator (processSample + prepare/reset)
│
├── Tests/
│   ├── TestMain.cpp             // Test runner entry point (for GRAINTests.jucer)
│   └── DSP/
│       ├── RMSDetectorTest.cpp
│       ├── DynamicBiasTest.cpp
│       ├── WaveshaperTest.cpp
│       ├── WarmthProcessorTest.cpp
│       ├── SpectralFocusTest.cpp
│       ├── DCBlockerTest.cpp
│       └── PipelineTest.cpp     // Integration tests (NEW)
│
├── PluginProcessor.h
├── PluginProcessor.cpp
├── PluginEditor.h
└── PluginEditor.cpp

GRAINTests.jucer                 // Separate Projucer project (consoleapp)
GRAIN.jucer                      // Main plugin project (audioplug)
```

---

## Blocks

### Block A — Split the monolith (P1 + P2)

Extract each abstraction from `GrainDSP.h` into its own header file inside `Source/DSP/`. Each module:
- Lives in the `GrainDSP` namespace
- Is header-only (no .cpp needed for current code)
- Includes only what it needs (`<cmath>`, `Constants.h`, etc.)
- Follows signal chain order in naming

#### Constants.h

Extract all `constexpr` values currently at the top of `GrainDSP.h`:

```cpp
#pragma once

namespace GrainDSP
{
// Math
constexpr float kTwoPi = 6.283185307f;

// RMS Detector
constexpr float kRmsAttackMs = 100.0f;
constexpr float kRmsReleaseMs = 300.0f;

// Dynamic Bias
constexpr float kBiasAmount = 0.3f;
constexpr float kBiasScale = 0.1f;
constexpr float kDCBlockerCutoffHz = 5.0f;

// Warmth
constexpr float kWarmthDepth = 0.22f;

// Spectral Focus
constexpr float kFocusLowShelfFreq = 200.0f;
constexpr float kFocusHighShelfFreq = 4000.0f;
constexpr float kFocusShelfGainDb = 3.0f;
constexpr float kFocusShelfQ = 0.707f;
}
```

#### DSPHelpers.h

Extract `calculateCoefficient()`, `applyMix()`, and `applyGain()` — small utility/pure functions that don't belong to any specific DSP stage:

```cpp
#pragma once
#include <cmath>

namespace GrainDSP
{
inline float calculateCoefficient(float sampleRate, float timeMs)
{
    return std::exp(-1.0f / (sampleRate * timeMs * 0.001f));
}

inline float applyMix(float dry, float wet, float mix)
{
    return dry + mix * (wet - dry);
}

inline float applyGain(float input, float gainLinear)
{
    return input * gainLinear;
}
}
```

#### RMSDetector.h

Extract the `RMSDetector` struct. Include `DSPHelpers.h` for `calculateCoefficient`.

#### DynamicBias.h

Extract `applyDynamicBias()`. Include `Constants.h` for `kBiasScale`.

#### Waveshaper.h

Extract `applyWaveshaper()`. No dependencies beyond `<cmath>`.

**Note:** `applyMix()` and `applyGain()` live in `DSPHelpers.h` (not `Waveshaper.h`) since they are general utility functions used by the pipeline, not specific DSP stages.

#### WarmthProcessor.h

Extract `applyWarmth()`. Include `Constants.h` for `kWarmthDepth`.

#### SpectralFocus.h

Extract `SpectralFocus` struct (with nested `BiquadState`), `FocusMode` enum, and the private coefficient calculation methods.

**Critical changes:**
- Refactor to mono pattern (see Block B).
- Replace `juce::MathConstants<float>::pi` with `kTwoPi * 0.5f` (from `Constants.h`) to eliminate the only JUCE dependency in the DSP code. This keeps all DSP headers pure C++ (`<cmath>` only) and allows the test binary and `generate_reference.cpp` to compile without JUCE include paths for DSP code.

#### DCBlocker.h

Extract `DCBlocker` struct. Include `Constants.h` for `kDCBlockerCutoffHz` and `kTwoPi`.

#### GrainDSPPipeline.h

This replaces the current free `processSample()` function. It becomes a struct/class that:
- Owns the stateful DSP modules (but NOT SpectralFocus stereo state — see below)
- Exposes `prepare()`, `reset()`, `processSample()`
- Handles the signal chain order

```cpp
#pragma once

#include "Constants.h"
#include "DCBlocker.h"
#include "DSPHelpers.h"
#include "DynamicBias.h"
#include "SpectralFocus.h"
#include "Waveshaper.h"
#include "WarmthProcessor.h"

namespace GrainDSP
{
struct DSPPipeline
{
    DCBlocker dcBlocker;
    SpectralFocus spectralFocus;

    void prepare(float sampleRate, FocusMode focusMode)
    {
        dcBlocker.prepare(sampleRate);
        spectralFocus.prepare(sampleRate, focusMode);
    }

    void reset()
    {
        dcBlocker.reset();
        spectralFocus.reset();
    }

    float processSample(float dry, float envelope, float drive,
                        float warmth, float mix, float gain)
    {
        const float biased = applyDynamicBias(dry, envelope, kBiasAmount);
        const float shaped = applyWaveshaper(biased, drive);
        const float warmed = applyWarmth(shaped, warmth);
        const float focused = spectralFocus.process(warmed);
        const float mixed = applyMix(dry, focused, mix);
        const float dcBlocked = dcBlocker.process(mixed);
        return applyGain(dcBlocked, gain);
    }
};
}
```

**Key difference from current code:** The pipeline is per-channel. `PluginProcessor` creates two instances: `pipelineLeft` and `pipelineRight`. This eliminates the `channel` parameter from `processSample()` and `spectralFocus.process()`.

#### Update PluginProcessor

```cpp
// Before (current)
GrainDSP::DCBlocker dcBlockerLeft;
GrainDSP::DCBlocker dcBlockerRight;
GrainDSP::SpectralFocus spectralFocus;

// After
GrainDSP::DSPPipeline pipelineLeft;
GrainDSP::DSPPipeline pipelineRight;
```

In `processBlock()`:
```cpp
// Before
leftChannel[sample] = GrainDSP::processSample(
    leftSample, currentEnvelope, drive, warmth, mix, gain,
    dcBlockerLeft, spectralFocus, 0);

// After
leftChannel[sample] = pipelineLeft.processSample(
    leftSample, currentEnvelope, drive, warmth, mix, gain);
```

---

### Block B — Unify stereo pattern to mono instances (P5)

**Decision: All DSP modules are mono. Stereo is managed by the Processor.**

Refactor `SpectralFocus` to remove internal `[2]` arrays:

```cpp
// Before (stereo internal)
struct SpectralFocus
{
    BiquadState lowShelf[2];
    BiquadState highShelf[2];
    float process(float input, int channel);
};

// After (mono)
struct SpectralFocus
{
    BiquadState lowShelf;
    BiquadState highShelf;
    float process(float input);

    void reset()
    {
        lowShelf.reset();
        highShelf.reset();
    }
};
```

Each `DSPPipeline` instance (one per channel) owns its own mono `SpectralFocus`. No more `channel` parameter anywhere.

---

### Block C — Fix bypass parameter type (P3)

Replace `AudioParameterFloat` with `AudioParameterBool`:

```cpp
// In parameter layout creation
params.push_back(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID("bypass", 1), "Bypass", false));
```

In `PluginProcessor.h`, change the pointer type:
```cpp
// Before
std::atomic<float>* bypassParam = nullptr;

// After
juce::AudioParameterBool* bypassParam = nullptr;
```

In `PluginProcessor.cpp`, update initialization and usage:
```cpp
// Initialization (constructor)
bypassParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("bypass"));

// Usage in processBlock
const bool bypass = bypassParam->get();
```

**Note on compatibility:** No presets have been published, so changing the parameter type is safe. Version stays at 1.

---

### Block D — Separate test target (P6)

Create a dedicated test runner as a **separate Projucer project** (`GRAINTests.jucer`).

**Why a separate `.jucer`?** Projucer does not support mixing project types. A `.jucer` file is either `audioplug` or `consoleapp`, never both. Projucer also overwrites the Xcode project on every save, so manually adding targets to `.xcodeproj` is not viable. A second `.jucer` of type `consoleapp` is the standard Projucer-compatible solution.

#### 1. Create `GRAINTests.jucer`

Create a new Projucer project:
- **Type:** Console Application
- **Name:** `GRAINTests`
- **Project folder:** Repository root (alongside `GRAIN.jucer`)
- **Modules:** `juce_core`, `juce_events` (minimum for `juce::UnitTest` + `ScopedJuceInitialiser_GUI`)
- **Module paths:** Same as `GRAIN.jucer` (e.g., `../../JUCE/modules`)
- **Header search paths:** Add `../Source` so test files can `#include "DSP/Constants.h"` etc.
- **Source files:**
  - `Source/Tests/TestMain.cpp` (compile=1)
  - `Source/Tests/DSP/*.cpp` (compile=1, one per module)
  - `Source/DSP/*.h` (compile=0, headers only)

**Builds output:** `Builds/MacOSX-Tests/` (separate from plugin build)

**Sync note:** Only `juce_core` and `juce_events` need to stay in sync with the main project. The DSP headers have no JUCE dependencies (only `<cmath>`), so the coupling is minimal.

#### 2. Create `Source/Tests/TestMain.cpp`

```cpp
#include <JuceHeader.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (runner.getResult(i)->failures > 0)
            failures += runner.getResult(i)->failures;
    }

    return failures > 0 ? 1 : 0;
}
```

#### 3. Split `DSPTests.cpp` into per-module test files

Each test file:
- Includes the corresponding DSP module header
- Registers its own `juce::UnitTest` subclass
- Is self-contained

Files to create in `Source/Tests/DSP/`:
- `RMSDetectorTest.cpp` — extract `runRMSDetectorTests()`
- `DynamicBiasTest.cpp` — extract `runDynamicBiasTests()`
- `WaveshaperTest.cpp` — extract `runWaveshaperTests()`
- `WarmthProcessorTest.cpp` — extract `runWarmthTests()`
- `SpectralFocusTest.cpp` — extract `runSpectralFocusTests()`
- `DCBlockerTest.cpp` — extract `runDCBlockerTests()` + `runDCOffsetAccumulationTest()`
- `PipelineTest.cpp` — NEW integration tests (see Block E)

#### 4. Remove hack from PluginProcessor.cpp

Delete these lines:
```cpp
#include "Tests/DSPTests.cpp"  // NOLINT(bugprone-suspicious-include)

#if JUCE_DEBUG
namespace
{
struct TestRunner
{
    TestRunner()
    {
        juce::UnitTestRunner runner;
        runner.runAllTests();
    }
} testRunner;
}
#endif
```

Delete the original `Source/Tests/DSPTests.cpp` after all tests have been migrated to the new per-module files.

#### 5. Update CI/build scripts

```bash
# Build and run tests (separate Xcode project)
xcodebuild -project Builds/MacOSX-Tests/GRAINTests.xcodeproj \
  -scheme "GRAINTests" -configuration Debug build

# Run — exit code 0 = pass, 1 = failures
./Builds/MacOSX-Tests/build/Debug/GRAINTests

# Build and validate plugin (now test-free)
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

---

### Block E — Integration tests for processSample / pipeline (P4)

Add `Tests/DSP/PipelineTest.cpp`:

```cpp
class PipelineTest : public juce::UnitTest
{
public:
    PipelineTest() : juce::UnitTest("GRAIN Pipeline Integration") {}

    void runTest() override
    {
        beginTest("Pipeline: silence in produces silence out");
        {
            GrainDSP::DSPPipeline pipeline;
            pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid);

            for (int i = 0; i < 512; ++i)
            {
                float result = pipeline.processSample(0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 1.0f);
                expectWithinAbsoluteError(result, 0.0f, 1e-5f);
            }
        }

        beginTest("Pipeline: mix=0 returns dry signal (sine)");
        {
            GrainDSP::DSPPipeline pipeline;
            pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid);

            // Use a sine wave instead of DC to avoid DC blocker convergence to 0
            float rmsInput = 0.0f;
            float rmsOutput = 0.0f;
            int numSamples = 4410;  // 100ms

            for (int i = 0; i < numSamples; ++i)
            {
                float phase = GrainDSP::kTwoPi * 440.0f * float(i) / 44100.0f;
                float input = 0.5f * std::sin(phase);
                float result = pipeline.processSample(input, 0.3f, 0.5f, 0.5f, 0.0f, 1.0f);
                rmsInput += input * input;
                rmsOutput += result * result;
            }

            rmsInput = std::sqrt(rmsInput / numSamples);
            rmsOutput = std::sqrt(rmsOutput / numSamples);

            // mix=0: output should closely match input (DC blocker has minimal effect on AC)
            expectWithinAbsoluteError(rmsOutput, rmsInput, 0.01f);
        }

        beginTest("Pipeline: no NaN/Inf on extreme input");
        {
            GrainDSP::DSPPipeline pipeline;
            pipeline.prepare(44100.0f, GrainDSP::FocusMode::High);

            float extremes[] = {0.0f, 1.0f, -1.0f, 100.0f, -100.0f, 1e-30f};
            for (float input : extremes)
            {
                float result = pipeline.processSample(input, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f);
                expect(!std::isnan(result));
                expect(!std::isinf(result));
            }
        }

        beginTest("Pipeline: output level approximately matches input at low settings");
        {
            GrainDSP::DSPPipeline pipeline;
            pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid);

            // Low drive, full mix, unity gain
            float rmsSum = 0.0f;
            float inputRmsSum = 0.0f;
            int numSamples = 44100;

            for (int i = 0; i < numSamples; ++i)
            {
                float phase = GrainDSP::kTwoPi * 440.0f * float(i) / 44100.0f;
                float input = 0.5f * std::sin(phase);
                float output = pipeline.processSample(input, 0.1f, 0.1f, 0.0f, 1.0f, 1.0f);
                rmsSum += output * output;
                inputRmsSum += input * input;
            }

            float outputRms = std::sqrt(rmsSum / numSamples);
            float inputRms = std::sqrt(inputRmsSum / numSamples);

            // At low settings, output should be within ~3dB of input
            expect(outputRms > inputRms * 0.5f);
            expect(outputRms < inputRms * 2.0f);
        }
    }
};

static PipelineTest pipelineTest;
```

---

## Execution Order

```
Block A (split monolith) → Block B (mono stereo) → Block C (bypass bool)
    → Block D (test target) → Block E (integration tests)
```

Blocks A and B can be done together since SpectralFocus is being both extracted and refactored. Block C is independent. Block D must be done before Block E (tests need somewhere to live).

---

## Acceptance Criteria

### Functional
- [ ] `GrainDSP.h` no longer exists (replaced by individual module files)
- [ ] All DSP modules are mono (no `channel` parameter, no internal `[2]` arrays)
- [ ] `DSPPipeline` struct owns per-channel stateful modules
- [ ] `PluginProcessor` creates two pipeline instances (L/R)
- [ ] Bypass parameter is `AudioParameterBool`
- [ ] File structure matches `docs/GRAIN_Code_Architecture.md`

### Tests
- [ ] `GRAINTests.jucer` (consoleapp) builds and runs independently
- [ ] Test binary returns exit code 0 on success, 1 on failure
- [ ] `PluginProcessor.cpp` has no `#include "Tests/..."` lines
- [ ] All existing tests pass in new structure
- [ ] New pipeline integration tests pass
- [ ] pluginval still passes on clean plugin binary

### Code Quality
- [ ] Each DSP header includes only what it needs
- [ ] No circular dependencies between DSP modules
- [ ] All files compile without warnings (`-Wall -Wextra`)
- [ ] clang-format passes on all new/modified files

---

## Regression Guard: Automated Before/After Comparison

The refactor must produce **bit-identical output** to the current code for the same input and parameters. This is verified automatically in two steps:

### Step 1: Generate reference output (BEFORE any code changes)

Before starting any refactor work, create and run a script that captures the current output as a binary reference file. This must be the **very first thing** done in this task.

Create `Tests/generate_reference.cpp`:

```cpp
#include "../Source/DSP/GrainDSP.h"
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>

int main()
{
    constexpr int kNumSamples = 44100;  // 1 second at 44.1kHz
    constexpr float kSampleRate = 44100.0f;
    constexpr float kFrequency = 440.0f;
    constexpr float kAmplitude = 0.5f;

    // Fixed parameters for reproducibility
    constexpr float kDrive = 0.5f;
    constexpr float kWarmth = 0.3f;
    constexpr float kMix = 1.0f;
    constexpr float kGain = 1.0f;

    // Initialize DSP (same as PluginProcessor::prepareToPlay)
    GrainDSP::RMSDetector rms;
    rms.prepare(kSampleRate, GrainDSP::kRmsAttackMs, GrainDSP::kRmsReleaseMs);

    GrainDSP::DCBlocker dcBlocker;
    dcBlocker.prepare(kSampleRate);

    GrainDSP::SpectralFocus focus;
    focus.prepare(kSampleRate, GrainDSP::FocusMode::Mid);

    // Process 1 second of 440Hz sine
    std::vector<float> output(kNumSamples);
    for (int i = 0; i < kNumSamples; ++i)
    {
        const float phase = GrainDSP::kTwoPi * kFrequency * float(i) / kSampleRate;
        const float input = kAmplitude * std::sin(phase);
        const float envelope = rms.process(input);

        output[i] = GrainDSP::processSample(
            input, envelope, kDrive, kWarmth, kMix, kGain,
            dcBlocker, focus, 0);
    }

    // Save to binary file
    const std::string path = "Tests/reference_output.bin";
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(output.data()),
               static_cast<std::streamsize>(output.size() * sizeof(float)));
    file.close();

    std::cout << "Reference output saved to " << path
              << " (" << kNumSamples << " samples)" << std::endl;

    return 0;
}
```

**Note on JUCE dependency:** `SpectralFocus` uses `juce::MathConstants<float>::pi` internally. After Block A, the refactored `SpectralFocus.h` should use `GrainDSP::kTwoPi / 2.0f` (from `Constants.h`) or `M_PI` from `<cmath>` instead, eliminating the JUCE dependency entirely. This allows `generate_reference.cpp` to compile without JUCE include paths.

Compile and run manually:
```bash
# Compile — no JUCE includes needed (DSP headers are pure C++)
clang++ -std=c++17 -I Source/ Tests/generate_reference.cpp -o generate_reference
./generate_reference
# Output: Tests/reference_output.bin
```

**Important:** Commit `reference_output.bin` to git so it survives the refactor. Delete it (and `generate_reference.cpp`) after the refactor is verified.

### Step 2: Regression test (AFTER refactor)

Add to `Tests/DSP/PipelineTest.cpp`:

```cpp
beginTest("Pipeline: regression guard — output matches pre-refactor reference");
{
    // Load reference
    std::ifstream file("Tests/reference_output.bin", std::ios::binary);
    expect(file.good());  // File must exist

    constexpr int kNumSamples = 44100;
    std::vector<float> reference(kNumSamples);
    file.read(reinterpret_cast<char*>(reference.data()),
              static_cast<std::streamsize>(kNumSamples * sizeof(float)));
    file.close();

    // Reproduce with new pipeline
    GrainDSP::DSPPipeline pipeline;
    pipeline.prepare(44100.0f, GrainDSP::FocusMode::Mid);

    GrainDSP::RMSDetector rms;
    rms.prepare(44100.0f, GrainDSP::kRmsAttackMs, GrainDSP::kRmsReleaseMs);

    for (int i = 0; i < kNumSamples; ++i)
    {
        const float phase = GrainDSP::kTwoPi * 440.0f * float(i) / 44100.0f;
        const float input = 0.5f * std::sin(phase);
        const float envelope = rms.process(input);
        const float result = pipeline.processSample(
            input, envelope, 0.5f, 0.3f, 1.0f, 1.0f);

        expectWithinAbsoluteError(result, reference[i], 1e-6f);
    }
}
```

If this test passes, the refactor is clean. If it fails, something changed the DSP behavior and must be investigated before proceeding.

### Step 3: Cleanup (AFTER test passes)

Remove these temporary files:
- `Tests/generate_reference.cpp`
- `Tests/reference_output.bin`

The regression test in `PipelineTest.cpp` can also be removed or converted to a standalone pipeline test without the binary reference.

---

## Out of Scope

- Oversampling (Task 007)
- GrainParameters class extraction (defer until UI task needs it)
- Moving to CMake (keep Projucer for now)
- Any DSP algorithm changes — this is purely structural
