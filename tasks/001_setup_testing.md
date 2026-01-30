# Task 002: Setup Testing Infrastructure

## Objective

Configure testing infrastructure before implementing DSP:
1. Verify pluginval works
2. Create unit test scaffold for DSP functions

## Prerequisites

- Task 001 NOT required first (tests can exist before implementation)
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

//==============================================================================
class GrainDSPTests : public juce::UnitTest
{
public:
    GrainDSPTests() : juce::UnitTest("GRAIN DSP Tests") {}

    void runTest() override
    {
        beginTest("Waveshaper: zero passthrough");
        expect(std::tanh(0.0f) == 0.0f);

        beginTest("Waveshaper: symmetry");
        {
            float x = 0.5f;
            expectWithinAbsoluteError(std::tanh(-x), -std::tanh(x), 0.0001f);
        }

        beginTest("Waveshaper: bounded output");
        {
            expect(std::tanh(100.0f) < 1.01f);
            expect(std::tanh(-100.0f) > -1.01f);
        }

        beginTest("Waveshaper: near-linear for small values");
        {
            float x = 0.1f;
            expectWithinAbsoluteError(std::tanh(x), x, 0.01f);
        }

        beginTest("Mix: full dry");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float mix = 0.0f;
            float result = (wet * mix) + (dry * (1.0f - mix));
            expectEquals(result, dry);
        }

        beginTest("Mix: full wet");
        {
            float dry = 1.0f;
            float wet = 0.5f;
            float mix = 1.0f;
            float result = (wet * mix) + (dry * (1.0f - mix));
            expectEquals(result, wet);
        }

        beginTest("Mix: 50/50 blend");
        {
            float dry = 1.0f;
            float wet = 0.0f;
            float mix = 0.5f;
            float result = (wet * mix) + (dry * (1.0f - mix));
            expectWithinAbsoluteError(result, 0.5f, 0.0001f);
        }

        beginTest("Gain: unity");
        {
            float input = 0.7f;
            float gain = 1.0f;
            expectEquals(input * gain, input);
        }

        beginTest("Gain: double");
        {
            float input = 0.5f;
            float gain = 2.0f;
            expectEquals(input * gain, 1.0f);
        }

        beginTest("Gain: zero (silence)");
        {
            float input = 0.7f;
            float gain = 0.0f;
            expectEquals(input * gain, 0.0f);
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

### 4. Alternative: Standalone Test Runner

Create `Source/Tests/TestMain.cpp` (optional, for CI):

```cpp
#include <JuceHeader.h>

int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    int numFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (runner.getResult(i)->failures > 0)
            numFailures++;
    }
    
    return numFailures > 0 ? 1 : 0;
}
```

---

## Acceptance Criteria

- [ ] pluginval installed and working
- [ ] Empty plugin passes pluginval validation
- [ ] `Source/Tests/DSPTests.cpp` created
- [ ] Tests added to Projucer project
- [ ] Tests run on Debug build (console shows results)
- [ ] All math tests pass

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Source/Tests/DSPTests.cpp` | Create |
| `GRAIN.jucer` | Add Tests group |
| `PluginProcessor.cpp` | Add test runner (Debug only) |

---

## Notes

- Tests use `std::tanh` directly for now
- When DSP functions are extracted to separate classes, tests will call those instead
- Test runner in PluginProcessor is temporary; remove before release

---

## Next Task

After testing infrastructure is ready → Task 001 (implement DSP)
