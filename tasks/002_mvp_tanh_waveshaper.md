# Task 001: MVP — tanh Waveshaper

## Objective

Implement the minimal viable DSP pipeline:

```
Input (Drive) → tanh waveshaper → Mix (Dry/Wet) → Output (Gain)
```

## Requirements

### Parameters

Create 3 parameters using `AudioProcessorValueTreeState`:

| ID | Name | Range | Default | Unit |
|----|------|-------|---------|------|
| `drive` | Drive | 0.0 – 1.0 | 0.5 | (normalized) |
| `mix` | Mix | 0.0 – 1.0 | 0.2 | (normalized) |
| `output` | Output | 0.0 – 2.0 | 1.0 | (linear gain) |

### DSP Logic in `processBlock()`

```cpp
// Pseudocode for each sample:
driveGain = 1.0 + (drive * 3.0)  // Maps 0-1 to 1x-4x gain
wetSample = tanh(inputSample * driveGain)
outputSample = (wetSample * mix) + (inputSample * (1.0 - mix))
outputSample *= outputGain
```

### Files to Modify

1. **`Source/PluginProcessor.h`**
   - Add `AudioProcessorValueTreeState` member
   - Add parameter layout function

2. **`Source/PluginProcessor.cpp`**
   - Initialize APVTS in constructor
   - Implement DSP in `processBlock()`

## Acceptance Criteria

- [ ] Plugin compiles without errors
- [ ] Plugin loads in DAW (Logic/Ableton)
- [ ] Three parameters visible in DAW automation
- [ ] Audio passes through (not silent)
- [ ] Drive increases saturation audibly at 100%
- [ ] Mix at 0% = dry signal only
- [ ] Mix at 100% = wet signal only
- [ ] Output controls final level
- [ ] No clicks/pops when changing parameters
- [ ] Bypass works cleanly (no level jump)

## Technical Notes

### Why `tanh`?

- Smooth saturation curve (no hard clipping)
- Odd harmonics only (symmetric)
- Stable output (bounded to ±1)
- CPU efficient

### Parameter Smoothing

Use `SmoothedValue<float>` for all parameters to avoid zipper noise:

```cpp
juce::SmoothedValue<float> driveSmoothed;
driveSmoothed.reset(sampleRate, 0.02);  // 20ms smoothing
```

### Stereo Processing

Process left and right channels identically (linked stereo):

```cpp
for (int channel = 0; channel < totalNumInputChannels; ++channel)
{
    auto* channelData = buffer.getWritePointer(channel);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // DSP here
    }
}
```

## Out of Scope (for this task)

- GUI controls (using generic DAW interface)
- RMS detector
- Dynamic bias
- Warmth control
- Focus (spectral) control
- Oversampling

## References

- `docs/DELIVERABLE_EN.md` — Full DSP specification
- `docs/DSP_ARCHITECTURE.md` — Signal flow diagra
