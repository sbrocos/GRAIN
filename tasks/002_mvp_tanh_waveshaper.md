# Task 002: MVP — tanh Waveshaper

## Objective

Implement the minimal viable DSP pipeline using pure functions (testable) with parameter smoothing:

```
Input (Drive) → tanh waveshaper → Mix (Dry/Wet) → Output (Gain)
```

---

## Prerequisites

- Task 001 completed (tests passing with stub implementations)

---

## Architecture

### Pure DSP Functions (in `Source/DSP/GrainDSP.h`)

```cpp
namespace GrainDSP
{
    inline float applyWaveshaper(float input, float drive)
    {
        float gained = input * (1.0f + drive * 3.0f);  // 1x to 4x gain
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
```

### processBlock (uses SmoothedValue + pure functions)

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, ...)
{
    float drive = driveSmoothed.getNextValue();
    float mix = mixSmoothed.getNextValue();
    float gain = gainSmoothed.getNextValue();
    bool bypass = *bypassParam < 0.5f ? false : true;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float dry = channelData[sample];
            float wet = GrainDSP::applyWaveshaper(dry, drive);
            channelData[sample] = GrainDSP::applyBypass(dry, wet, bypass, mix);
            channelData[sample] = GrainDSP::applyGain(channelData[sample], gain);
        }
    }
}
```

---

## Parameters

Create using `AudioProcessorValueTreeState`:

| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `drive` | Drive | 0.0 – 1.0 | 0.5 | Normalized, maps to 1x–4x gain internally |
| `mix` | Mix | 0.0 – 1.0 | 0.2 | Normalized (0% = dry, 100% = wet) |
| `output` | Output | -12.0 – +12.0 dB | 0.0 | In dB, convert to linear in process |
| `bypass` | Bypass | 0.0 – 1.0 | 0.0 | 0 = off, 1 = on (acts as bool) |

### Parameter Setup

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bypass", "Bypass",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f),
        0.0f
    ));

    return { params.begin(), params.end() };
}
```

### dB to Linear Conversion

```cpp
float outputGain = juce::Decibels::decibelsToGain(outputParam->load());
```

---

## Parameter Smoothing

Use `SmoothedValue<float>` for all continuous parameters:

```cpp
// In PluginProcessor.h
juce::SmoothedValue<float> driveSmoothed;
juce::SmoothedValue<float> mixSmoothed;
juce::SmoothedValue<float> gainSmoothed;

// In prepareToPlay()
driveSmoothed.reset(sampleRate, 0.02);  // 20ms smoothing
mixSmoothed.reset(sampleRate, 0.02);
gainSmoothed.reset(sampleRate, 0.02);

// In processBlock() - at start of block
driveSmoothed.setTargetValue(*driveParam);
mixSmoothed.setTargetValue(*mixParam);
gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(*outputParam));
```

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Source/DSP/GrainDSP.h` | Create (pure functions) |
| `Source/PluginProcessor.h` | Add APVTS, SmoothedValues |
| `Source/PluginProcessor.cpp` | Initialize APVTS, implement processBlock |
| `Source/Tests/DSPTests.cpp` | Update includes to use `GrainDSP.h` |
| `GRAIN.jucer` | Add DSP group |

---

## Folder Structure After Task

```
Source/
├── DSP/
│   └── GrainDSP.h          # Pure functions
├── Tests/
│   └── DSPTests.cpp        # Unit tests
├── PluginProcessor.h
├── PluginProcessor.cpp
├── PluginEditor.h
└── PluginEditor.cpp
```

---

## Acceptance Criteria

### Functional
- [ ] Plugin compiles without errors
- [ ] Plugin loads in DAW (Logic/Ableton)
- [ ] Four parameters visible in DAW automation (Drive, Mix, Output, Bypass)
- [ ] Audio passes through (not silent)
- [ ] Drive increases saturation audibly at 100%
- [ ] Mix at 0% = dry signal only
- [ ] Mix at 100% = wet signal only
- [ ] Output controls final level in dB
- [ ] Bypass returns pure dry signal

### Quality
- [ ] No clicks/pops when changing parameters (smoothing works)
- [ ] Bypass works cleanly (no level jump)
- [ ] pluginval passes

### Tests
- [ ] All unit tests from Task 001 still pass
- [ ] Tests now use `GrainDSP.h` instead of inline stubs

---

## Testing Checklist

```bash
# 1. Build
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 2. Run pluginval
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 3. Check unit tests (in console output when plugin loads)

# 4. Manual test in DAW
# - Load on track with audio
# - Automate Drive 0→100% quickly → no clicks
# - Toggle Bypass → no level jump
# - Mix at 0% → dry only
# - Mix at 100% → wet only
```

---

## Technical Notes

### Why Pure Functions?

1. **Testable** — Unit tests don't need SmoothedValue
2. **Reusable** — Same math in processBlock and tests
3. **Clear** — Separation of concerns (math vs. smoothing)

### Why Bypass as Parameter (not host bypass)?

1. **Smooth transition** — SmoothedValue prevents clicks
2. **Testable** — Can unit test bypass behavior
3. **Consistent** — Same behavior in all DAWs

### dB for Output Gain

- More intuitive for users (+6dB = double, -6dB = half)
- Industry standard for gain controls
- Easy conversion: `Decibels::decibelsToGain()`

---

## Out of Scope (future tasks)

- RMS detector
- Dynamic bias
- Warmth control
- Focus (spectral) control
- Oversampling
- GUI
