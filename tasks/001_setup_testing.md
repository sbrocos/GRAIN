# Task 001: Setup Testing Infrastructure

## Objective

Configure testing infrastructure before implementing DSP (TDD approach):
1. Verify pluginval works
2. Create unit test scaffold for DSP functions

---

## Testing Philosophy

**Separate pure DSP functions from parameter smoothing:**

| What | How to test |
|------|-------------|
| DSP logic (tanh, mix, gain) | Unit tests (pure functions) |
| Parameter smoothing | Discontinuity test + listening test |
| Plugin stability | pluginval |

This avoids flaky tests that depend on timing/smoothing convergence.

---

## Test Constants

```cpp
namespace TestConstants
{
    constexpr float TOLERANCE = 1e-5f;           // For math operations
    constexpr float CLICK_THRESHOLD = 0.01f;     // -40dB, for discontinuity detection
    constexpr int TEST_BUFFER_SIZE = 512;
}
```

---

## Prerequisites

- Plugin compiles (skeleton from Projucer)

---

## Part A: pluginval Setup

### 1. Install

```bash
brew install pluginval
```

### 2. Verify Installation

```bash
pluginval --version
```

### 3. Run Against Current Build

```bash
# Build first
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# Validate
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

### Expected Result

Plugin should pass basic validation (even with no DSP yet).

---

## Part B: Unit Test Scaffold

### 1. Create Test File

Create `Source/Tests/DSPTests.cpp`:

```cpp
#include <JuceHeader.h>
#include <cmath>

//==============================================================================
namespace TestConstants
{
    constexpr float TOLERANCE = 1e-5f;
    constexpr float CLICK_THRESHOLD = 0.01f;
    constexpr int BUFFER_SIZE = 512;
}

//==============================================================================
// Pure DSP functions (to be implemented in Task 002)
// These declarations will match the actual implementation

namespace GrainDSP
{
    inline float applyWaveshaper(float input, float drive)
    {
        float gained = input * (1.0f + drive * 3.0f);
        return std::tanh(gained);
    }

    inline float applyMix(float dry, float wet, float mix)
    {
        return (wet * mix) + (dry * (1.0f - mix));
    }

    inline float applyGain(float input, float gainLinear)
    {
        return input * gainLinear;
    }
    
    inline float applyBypass(float dry, float wet, bool bypass, float mix)
    {
        float effectiveMix = bypass ? 0.0f : mix;
        return applyMix(dry, wet, effectiveMix);
    }
}

//==============================================================================
class GrainDSPTests : public juce::UnitTest
{
public:
    GrainDSPTests() : juce::UnitTest("GRAIN DSP Tests") {}

    void runTest() override
    {
        runWaveshaperTests();
        runMixTests();
        runGainTests();
        runBypassTests();
        runBufferTests();
        runDiscontinuityTests();
    }

private:
    //==========================================================================
    void runWaveshaperTests()
    {
        beginTest("Waveshaper: zero passthrough");
        expectWithinAbsoluteError(GrainDSP::applyWaveshaper(0.0f, 0.0f), 0.0f, TestConstants::TOLERANCE);

        beginTest("Waveshaper: symmetry");
        {
            float x = 0.5f;
            float drive = 0.5f;
            expectWithinAbsoluteError(
                GrainDSP::applyWaveshaper(-x, drive), 
                -GrainDSP::applyWaveshaper(x, drive), 
                TestConstants::TOLERANCE
            );
        }

        beginTest("Waveshaper: bounded output");
        {
            expect(GrainDSP::applyWaveshaper(100.0f, 1.0f) < 1.01f);
            expect(GrainDSP::applyWaveshaper(-100.0f, 1.0f) > -1.01f);
        }

        beginTest("Waveshaper: near-linear for small values");
        {
            float x = 0.05f;
            float result = GrainDSP::applyWaveshaper(x, 0.0f);  // drive = 0, gain = 1x
            expectWithinAbsoluteError(result, x, 0.01f);
        }
    }

    //==========================================================================
    void runMixTests()
    {
        beginTest("Mix: full dry (mix = 0)");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float result = GrainDSP::applyMix(dry, wet, 0.0f);
            expectWithinAbsoluteError(result, dry, TestConstants::TOLERANCE);
        }

        beginTest("Mix: full wet (mix = 1)");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float result = GrainDSP::applyMix(dry, wet, 1.0f);
            expectWithinAbsoluteError(result, wet, TestConstants::TOLERANCE);
        }

        beginTest("Mix: 50/50 blend");
        {
            float dry = 1.0f;
            float wet = 0.0f;
            float result = GrainDSP::applyMix(dry, wet, 0.5f);
            expectWithinAbsoluteError(result, 0.5f, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runGainTests()
    {
        beginTest("Gain: unity");
        {
            float input = 0.7f;
            float result = GrainDSP::applyGain(input, 1.0f);
            expectWithinAbsoluteError(result, input, TestConstants::TOLERANCE);
        }

        beginTest("Gain: double (+6dB)");
        {
            float input = 0.5f;
            float result = GrainDSP::applyGain(input, 2.0f);
            expectWithinAbsoluteError(result, 1.0f, TestConstants::TOLERANCE);
        }

        beginTest("Gain: zero (silence)");
        {
            float input = 0.7f;
            float result = GrainDSP::applyGain(input, 0.0f);
            expectWithinAbsoluteError(result, 0.0f, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runBypassTests()
    {
        beginTest("Bypass: output equals input when bypassed");
        {
            float dry = 0.7f;
            float wet = 0.3f;  // Different from dry
            float result = GrainDSP::applyBypass(dry, wet, true, 0.5f);
            expectWithinAbsoluteError(result, dry, TestConstants::TOLERANCE);
        }

        beginTest("Bypass: normal processing when not bypassed");
        {
            float dry = 0.7f;
            float wet = 0.3f;
            float mix = 0.5f;
            float result = GrainDSP::applyBypass(dry, wet, false, mix);
            float expected = GrainDSP::applyMix(dry, wet, mix);
            expectWithinAbsoluteError(result, expected, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runBufferTests()
    {
        beginTest("Buffer processing: stability with constant input");
        {
            std::vector<float> buffer(TestConstants::BUFFER_SIZE, 0.5f);
            float drive = 0.5f;
            float mix = 0.5f;
            float gain = 1.0f;

            // Process buffer
            for (int i = 0; i < TestConstants::BUFFER_SIZE; ++i)
            {
                float dry = buffer[i];
                float wet = GrainDSP::applyWaveshaper(dry, drive);
                float mixed = GrainDSP::applyMix(dry, wet, mix);
                buffer[i] = GrainDSP::applyGain(mixed, gain);
            }

            // All samples should be identical (constant input -> constant output)
            float expected = buffer[0];
            for (int i = 1; i < TestConstants::BUFFER_SIZE; ++i)
            {
                expectWithinAbsoluteError(buffer[i], expected, TestConstants::TOLERANCE);
            }
        }

        beginTest("Buffer processing: no state leak between samples");
        {
            float drive = 0.5f;
            
            // Process same value multiple times
            float result1 = GrainDSP::applyWaveshaper(0.5f, drive);
            float result2 = GrainDSP::applyWaveshaper(0.5f, drive);
            float result3 = GrainDSP::applyWaveshaper(0.5f, drive);
            
            expectWithinAbsoluteError(result1, result2, TestConstants::TOLERANCE);
            expectWithinAbsoluteError(result2, result3, TestConstants::TOLERANCE);
        }
    }

    //==========================================================================
    void runDiscontinuityTests()
    {
        beginTest("Parameter change: no discontinuities on silent input");
        {
            // Silent input - any click would be clearly audible
            std::vector<float> buffer(TestConstants::BUFFER_SIZE, 0.0f);
            
            float prevSample = 0.0f;
            bool hasClick = false;

            // Simulate abrupt parameter change: drive 0 -> 1
            for (int i = 0; i < TestConstants::BUFFER_SIZE; ++i)
            {
                // Abrupt change at midpoint
                float drive = (i < TestConstants::BUFFER_SIZE / 2) ? 0.0f : 1.0f;
                
                float wet = GrainDSP::applyWaveshaper(buffer[i], drive);
                float output = GrainDSP::applyMix(buffer[i], wet, 0.5f);
                
                float delta = std::abs(output - prevSample);
                if (delta > TestConstants::CLICK_THRESHOLD)
                {
                    hasClick = true;
                }
                
                prevSample = output;
            }

            // Note: This test will PASS with pure functions (no state = no clicks on silence)
            // The real discontinuity test requires SmoothedValue in processBlock
            // This test documents the expected behavior
            expect(!hasClick);
        }
    }
};

//==============================================================================
// Register the test
static GrainDSPTests grainDSPTests;
```

### 2. Add to Projucer

1. Open `GRAIN.jucer` in Projucer
2. Right-click on `Source` → Add New Group → name it `Tests`
3. Right-click on `Tests` → Add Existing Files → select `DSPTests.cpp`
4. Save and re-export to Xcode

### 3. Create Test Runner

Add to `PluginProcessor.cpp` (temporary, for development):

```cpp
// At the top of the file, after includes:
#if JUCE_DEBUG
static struct TestRunner
{
    TestRunner()
    {
        juce::UnitTestRunner runner;
        runner.runAllTests();
    }
} testRunner;
#endif
```

This runs tests automatically when plugin loads in Debug mode.

---

## Acceptance Criteria

- [ ] pluginval installed and working
- [ ] Empty plugin passes pluginval validation
- [ ] `Source/Tests/DSPTests.cpp` created
- [ ] Tests added to Projucer project
- [ ] Tests run on Debug build (console shows results)
- [ ] All tests pass (with inline stub implementations)

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Source/Tests/DSPTests.cpp` | Create |
| `GRAIN.jucer` | Add Tests group |
| `PluginProcessor.cpp` | Add test runner (Debug only) |

---

## Notes

- Tests use inline implementations initially
- Task 002 will extract these to `Source/DSP/` and `processBlock` will call them
- Test runner in PluginProcessor is temporary; remove before release
- Discontinuity test on pure functions always passes (no state); real smoothing test is via listening

---

## Next Task

After testing infrastructure is ready → Task 002 (implement DSP)
