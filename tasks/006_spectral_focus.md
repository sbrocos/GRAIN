# Task 006: Spectral Focus (Low / Mid / High)

## Objective

Implement a discrete spectral focus control that gently biases where harmonic generation is emphasized. This is NOT a traditional EQ — it shapes the saturation character by applying gentle filtering before or after the nonlinear stages.

```
... → Warmth → [Focus] → Mix → ...
```

---

## Prerequisites

- Task 005 completed (Warmth control working)

---

## Design Rationale

### What Is Focus?

Focus determines the spectral region where saturation harmonics are most prominent:

| Mode | Character | Use case |
|------|-----------|----------|
| **Low** | Thicker, heavier bottom end | Drum bus, bass-heavy material |
| **Mid** | Balanced, presence emphasis | Vocals, general mix |
| **High** | Airy, crisp top end | Acoustic, bright mixes |

### Why Discrete Modes (Not Continuous)?

From the PRD:
> "Discrete spectral focus (Low/Mid/High)"

- Simpler UX — one selector, not multiple knobs
- Prevents "bad" settings — each mode is pre-calibrated
- Aligns with GRAIN's "no technical decisions for user" philosophy

### Why Post-Waveshaper?

Placing focus after the saturation stages means it shapes the **output** harmonics, not the input signal. This avoids changing the waveshaper's operating point and keeps saturation behavior consistent regardless of focus mode.

---

## Architecture

### Filter Design

Each mode uses a pair of shelf filters (low shelf + high shelf) to create a gentle spectral tilt:

| Mode | Low Shelf (200 Hz) | High Shelf (4 kHz) | Net Effect |
|------|--------------------|--------------------|------------|
| **Low** | +2 dB | -2 dB | Emphasis below 200 Hz |
| **Mid** | -1 dB | -1 dB | Emphasis 200 Hz – 4 kHz |
| **High** | -2 dB | +2 dB | Emphasis above 4 kHz |

**Note:** These are very gentle gains. The focus is subtle spectral bias, not dramatic EQ.

### Pure Functions and State (in `Source/DSP/GrainDSP.h`)

```cpp
namespace GrainDSP
{
    namespace Constants
    {
        // Existing constants...
        
        // Focus: shelf filter parameters
        constexpr float kFocusLowShelfFreq = 200.0f;    // Hz
        constexpr float kFocusHighShelfFreq = 4000.0f;   // Hz
        constexpr float kFocusShelfGainDb = 2.0f;        // dB (max boost/cut)
        constexpr float kFocusShelfQ = 0.707f;           // Butterworth Q
    }

    enum class FocusMode
    {
        Low = 0,
        Mid = 1,
        High = 2
    };

    // Focus filter state (needs to persist between samples)
    struct SpectralFocus
    {
        // Biquad filter coefficients
        struct BiquadState
        {
            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
            float a1 = 0.0f, a2 = 0.0f;
            
            // State variables (per channel)
            float z1 = 0.0f, z2 = 0.0f;
            
            float process(float input)
            {
                float output = b0 * input + z1;
                z1 = b1 * input - a1 * output + z2;
                z2 = b2 * input - a2 * output;
                return output;
            }
            
            void reset()
            {
                z1 = 0.0f;
                z2 = 0.0f;
            }
        };

        // Two filters per channel: low shelf + high shelf
        // Stereo: [0] = left, [1] = right
        BiquadState lowShelf[2];
        BiquadState highShelf[2];

        void prepare(float sampleRate, FocusMode mode)
        {
            float lowGainDb = 0.0f;
            float highGainDb = 0.0f;
            
            switch (mode)
            {
                case FocusMode::Low:
                    lowGainDb = Constants::kFocusShelfGainDb;    // +2 dB
                    highGainDb = -Constants::kFocusShelfGainDb;  // -2 dB
                    break;
                    
                case FocusMode::Mid:
                    lowGainDb = -Constants::kFocusShelfGainDb * 0.5f;  // -1 dB
                    highGainDb = -Constants::kFocusShelfGainDb * 0.5f; // -1 dB
                    break;
                    
                case FocusMode::High:
                    lowGainDb = -Constants::kFocusShelfGainDb;   // -2 dB
                    highGainDb = Constants::kFocusShelfGainDb;   // +2 dB
                    break;
            }
            
            // Calculate coefficients for both channels (identical)
            auto lowCoeffs = calculateLowShelf(
                sampleRate,
                Constants::kFocusLowShelfFreq,
                Constants::kFocusShelfQ,
                lowGainDb
            );
            
            auto highCoeffs = calculateHighShelf(
                sampleRate,
                Constants::kFocusHighShelfFreq,
                Constants::kFocusShelfQ,
                highGainDb
            );
            
            for (int ch = 0; ch < 2; ++ch)
            {
                lowShelf[ch].b0 = lowCoeffs.b0;
                lowShelf[ch].b1 = lowCoeffs.b1;
                lowShelf[ch].b2 = lowCoeffs.b2;
                lowShelf[ch].a1 = lowCoeffs.a1;
                lowShelf[ch].a2 = lowCoeffs.a2;
                
                highShelf[ch].b0 = highCoeffs.b0;
                highShelf[ch].b1 = highCoeffs.b1;
                highShelf[ch].b2 = highCoeffs.b2;
                highShelf[ch].a1 = highCoeffs.a1;
                highShelf[ch].a2 = highCoeffs.a2;
            }
        }
        
        float process(float input, int channel)
        {
            float output = lowShelf[channel].process(input);
            output = highShelf[channel].process(output);
            return output;
        }
        
        void reset()
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                lowShelf[ch].reset();
                highShelf[ch].reset();
            }
        }

    private:
        struct Coefficients { float b0, b1, b2, a1, a2; };
        
        // Low shelf biquad coefficient calculation
        // Reference: Audio EQ Cookbook (Robert Bristow-Johnson)
        static Coefficients calculateLowShelf(
            float sampleRate, float freq, float Q, float gainDb)
        {
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * juce::MathConstants<float>::pi * freq / sampleRate;
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);
            float sqrtA = std::sqrt(A);
            
            float a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
            
            Coefficients c;
            c.b0 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha)) / a0;
            c.b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
            c.b2 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha)) / a0;
            c.a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
            c.a2 = ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
            
            return c;
        }
        
        // High shelf biquad coefficient calculation
        static Coefficients calculateHighShelf(
            float sampleRate, float freq, float Q, float gainDb)
        {
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * juce::MathConstants<float>::pi * freq / sampleRate;
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);
            float sqrtA = std::sqrt(A);
            
            float a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
            
            Coefficients c;
            c.b0 = (A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha)) / a0;
            c.b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
            c.b2 = (A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha)) / a0;
            c.a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
            c.a2 = ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
            
            return c;
        }
    };
}
```

### Alternative: Use JUCE's Built-in Filters

```cpp
// Simpler approach using juce::dsp::IIR
juce::dsp::IIR::Filter<float> lowShelf;
juce::dsp::IIR::Filter<float> highShelf;

// In prepare():
auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
    sampleRate, 200.0f, 0.707f, juce::Decibels::decibelsToGain(2.0f));
lowShelf.coefficients = lowCoeffs;
```

**Recommendation:** Use JUCE's built-in filters if available in your JUCE module configuration. Fall back to manual biquad if `juce_dsp` module is not included.

---

## Integration in processBlock

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, ...)
{
    // Check if focus mode changed
    FocusMode currentFocus = static_cast<FocusMode>(static_cast<int>(*focusParam));
    if (currentFocus != lastFocusMode)
    {
        spectralFocus.prepare(getSampleRate(), currentFocus);
        lastFocusMode = currentFocus;
        // Note: don't reset filter state to avoid click on mode change
    }

    // Update targets at block start
    driveSmoothed.setTargetValue(*driveParam);
    biasSmoothed.setTargetValue(*biasParam);
    warmthSmoothed.setTargetValue(*warmthParam);
    
    bool bypass = *bypassParam > 0.5f;
    float targetMix = bypass ? 0.0f : static_cast<float>(*mixParam);
    mixSmoothed.setTargetValue(targetMix);
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(*outputParam));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float drive = driveSmoothed.getNextValue();
            float biasAmount = biasSmoothed.getNextValue();
            float warmth = warmthSmoothed.getNextValue();
            float mix = mixSmoothed.getNextValue();
            float gain = gainSmoothed.getNextValue();
            
            float dry = channelData[sample];
            
            // RMS detection (linked stereo)
            float rmsLevel = rmsDetector.process(dry);
            
            // Dynamic bias
            float biased = GrainDSP::applyDynamicBias(dry, rmsLevel, biasAmount);
            
            // Waveshaper
            float shaped = GrainDSP::applyWaveshaper(biased, drive);
            
            // Warmth
            float warmed = GrainDSP::applyWarmth(shaped, warmth, 0.0f);
            
            // Spectral Focus (NEW)
            float focused = spectralFocus.process(warmed, channel);
            
            // Mix + gain
            float mixed = GrainDSP::applyMix(dry, focused, mix);
            channelData[sample] = GrainDSP::applyGain(mixed, gain);
        }
    }
}
```

---

## New Parameter

Add to `AudioProcessorValueTreeState`:

| ID | Name | Range | Default | Notes |
|----|------|-------|---------|-------|
| `focus` | Focus | 0, 1, 2 | 1 (Mid) | 0=Low, 1=Mid, 2=High |

```cpp
params.push_back(std::make_unique<juce::AudioParameterChoice>(
    "focus", "Focus",
    juce::StringArray { "Low", "Mid", "High" },
    1  // Default: Mid
));
```

**Note:** Using `AudioParameterChoice` instead of `AudioParameterFloat` for discrete selection.

---

## Mode Change Handling

Switching focus modes recalculates filter coefficients. To avoid clicks:

1. **Don't reset filter state** — Let the filter naturally transition with new coefficients
2. **Recalculate only on change** — Compare with last mode, avoid unnecessary recalculation

```cpp
// In PluginProcessor.h
FocusMode lastFocusMode = FocusMode::Mid;
GrainDSP::SpectralFocus spectralFocus;

// In prepareToPlay()
spectralFocus.prepare(sampleRate, FocusMode::Mid);
spectralFocus.reset();
lastFocusMode = FocusMode::Mid;
```

---

## Unit Tests

Add to `Source/Tests/DSPTests.cpp`:

```cpp
void runSpectralFocusTests()
{
    beginTest("Focus: Mid mode is near-unity for broadband signal");
    {
        GrainDSP::SpectralFocus focus;
        focus.prepare(44100.0f, GrainDSP::FocusMode::Mid);
        
        // Process DC (should pass through with minimal change)
        // Note: shelves at -1dB each will attenuate slightly
        float input = 0.5f;
        float result = 0.0f;
        
        // Let filter settle
        for (int i = 0; i < 1000; ++i)
        {
            result = focus.process(input, 0);
        }
        
        // Mid mode cuts both shelves by -1dB, so DC should be slightly reduced
        // -1dB low + -1dB high ≈ -2dB total on DC ≈ 0.79x
        expect(result > 0.3f);  // Not completely attenuated
        expect(result < 0.6f);  // Not boosted
    }

    beginTest("Focus: reset clears filter state");
    {
        GrainDSP::SpectralFocus focus;
        focus.prepare(44100.0f, GrainDSP::FocusMode::Low);
        
        // Build up filter state
        for (int i = 0; i < 1000; ++i)
        {
            focus.process(0.5f, 0);
        }
        
        focus.reset();
        
        // After reset, processing silence should produce silence
        float result = 0.0f;
        for (int i = 0; i < 100; ++i)
        {
            result = focus.process(0.0f, 0);
        }
        
        expectWithinAbsoluteError(result, 0.0f, TestConstants::TOLERANCE);
    }

    beginTest("Focus: silence in produces silence out");
    {
        GrainDSP::SpectralFocus focus;
        focus.prepare(44100.0f, GrainDSP::FocusMode::High);
        
        for (int i = 0; i < 100; ++i)
        {
            float result = focus.process(0.0f, 0);
            expectWithinAbsoluteError(result, 0.0f, TestConstants::TOLERANCE);
        }
    }

    beginTest("Focus: Low mode boosts low frequencies");
    {
        GrainDSP::SpectralFocus focus;
        focus.prepare(44100.0f, GrainDSP::FocusMode::Low);
        
        // Generate low frequency sine (100 Hz)
        float freq = 100.0f;
        float sampleRate = 44100.0f;
        float rmsLow = 0.0f;
        int numSamples = static_cast<int>(sampleRate);  // 1 second
        
        // Let filter settle, then measure RMS of last portion
        int measureStart = numSamples / 2;
        
        for (int i = 0; i < numSamples; ++i)
        {
            float input = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * freq * i / sampleRate);
            float output = focus.process(input, 0);
            
            if (i >= measureStart)
            {
                rmsLow += output * output;
            }
        }
        rmsLow = std::sqrt(rmsLow / (numSamples - measureStart));
        
        // Now test high frequency (8000 Hz) with same mode
        focus.reset();
        float rmsHigh = 0.0f;
        freq = 8000.0f;
        
        for (int i = 0; i < numSamples; ++i)
        {
            float input = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * freq * i / sampleRate);
            float output = focus.process(input, 0);
            
            if (i >= measureStart)
            {
                rmsHigh += output * output;
            }
        }
        rmsHigh = std::sqrt(rmsHigh / (numSamples - measureStart));
        
        // In Low mode: low frequency RMS should be greater than high frequency RMS
        expect(rmsLow > rmsHigh);
    }

    beginTest("Focus: High mode boosts high frequencies");
    {
        GrainDSP::SpectralFocus focus;
        focus.prepare(44100.0f, GrainDSP::FocusMode::High);
        
        float sampleRate = 44100.0f;
        int numSamples = static_cast<int>(sampleRate);
        int measureStart = numSamples / 2;
        
        // Low frequency (100 Hz)
        float rmsLow = 0.0f;
        float freq = 100.0f;
        
        for (int i = 0; i < numSamples; ++i)
        {
            float input = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * freq * i / sampleRate);
            float output = focus.process(input, 0);
            
            if (i >= measureStart)
                rmsLow += output * output;
        }
        rmsLow = std::sqrt(rmsLow / (numSamples - measureStart));
        
        // High frequency (8000 Hz)
        focus.reset();
        float rmsHigh = 0.0f;
        freq = 8000.0f;
        
        for (int i = 0; i < numSamples; ++i)
        {
            float input = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * freq * i / sampleRate);
            float output = focus.process(input, 0);
            
            if (i >= measureStart)
                rmsHigh += output * output;
        }
        rmsHigh = std::sqrt(rmsHigh / (numSamples - measureStart));
        
        // In High mode: high frequency RMS should be greater than low frequency RMS
        expect(rmsHigh > rmsLow);
    }

    beginTest("Focus: stereo channels are independent");
    {
        GrainDSP::SpectralFocus focus;
        focus.prepare(44100.0f, GrainDSP::FocusMode::Low);
        
        // Process different signals on L and R
        float resultL = 0.0f;
        float resultR = 0.0f;
        
        for (int i = 0; i < 100; ++i)
        {
            resultL = focus.process(0.5f, 0);   // Left: constant
            resultR = focus.process(-0.5f, 1);   // Right: inverted
        }
        
        // Outputs should be different (independent state)
        expect(std::abs(resultL + resultR) > TestConstants::TOLERANCE);
    }

    beginTest("Focus: no NaN or Inf on edge cases");
    {
        GrainDSP::SpectralFocus focus;
        focus.prepare(44100.0f, GrainDSP::FocusMode::Low);
        
        // Test extreme values
        float result = focus.process(1.0f, 0);
        expect(!std::isnan(result));
        expect(!std::isinf(result));
        
        result = focus.process(-1.0f, 0);
        expect(!std::isnan(result));
        expect(!std::isinf(result));
        
        result = focus.process(0.0f, 0);
        expect(!std::isnan(result));
        expect(!std::isinf(result));
    }
}
```

Don't forget to call `runSpectralFocusTests()` from `runTest()`.

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Source/DSP/GrainDSP.h` | Add SpectralFocus struct, FocusMode enum, constants |
| `Source/PluginProcessor.h` | Add focus parameter, SpectralFocus member |
| `Source/PluginProcessor.cpp` | Integrate focus in processBlock |
| `Source/Tests/DSPTests.cpp` | Add spectral focus tests |

---

## Signal Flow After This Task

```
Input → RMS Detector → envelope
   ↓                      ↓
   └──→ Dynamic Bias ←────┘
            ↓
       Waveshaper (tanh)
            ↓
         Warmth
            ↓
      Spectral Focus      ← NEW
            ↓
          Mix
            ↓
          Gain
            ↓
         Output
```

---

## Acceptance Criteria

### Functional
- [ ] Focus compiles without errors
- [ ] Three modes selectable (Low/Mid/High)
- [ ] Low mode emphasizes low frequencies
- [ ] High mode emphasizes high frequencies
- [ ] Mid mode is relatively neutral
- [ ] Mode switching doesn't produce clicks

### Quality
- [ ] pluginval still passes
- [ ] All previous tests still pass
- [ ] New focus tests pass
- [ ] No NaN/Inf on edge cases
- [ ] Stereo channels process independently

### Listening
- [ ] Low: thicker bottom end, less sparkle
- [ ] Mid: balanced, natural presence
- [ ] High: more air, less weight
- [ ] All modes: subtle effect, not dramatic EQ
- [ ] Mode switching: smooth transition, no click

---

## Testing Checklist

```bash
# 1. Build
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 2. Run pluginval
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 3. Check unit tests (console output)

# 4. Manual listening test
# - Load plugin on full mix
# - Set mix = 50%, drive = 50%
# - Toggle Low → Mid → High
# - Listen for: subtle spectral shift
# - Verify: no clicks on mode change
# - Verify: each mode has distinct but gentle character
```

---

## Technical Notes

### Shelf Filter Gains

The ±2dB gains are intentionally mild:

| Gain | Perceived effect |
|------|-----------------|
| ±1 dB | Barely perceptible |
| **±2 dB** | Subtle but audible |
| ±4 dB | Noticeable, too strong for GRAIN |

May need adjustment after listening tests. Constants are in `GrainDSP::Constants`.

### JUCE `juce_dsp` Module

If `juce_dsp` is included in the project, consider using:
- `juce::dsp::IIR::Filter` for biquad implementation
- `juce::dsp::IIR::Coefficients` for coefficient calculation

This would simplify the code but adds a module dependency. The manual biquad implementation has no dependencies.

### Mode Change Without Reset

When switching modes, we recalculate coefficients but DON'T reset filter state (`z1`, `z2`). This creates a natural transition as the filter adapts to new coefficients over a few milliseconds. Resetting would cause a discontinuity (click).

---

## Out of Scope (future tasks)

- Oversampling (Task 007)
- Continuous focus parameter (instead of discrete)
- Custom frequency/gain per mode
- Focus applied pre-waveshaper (alternative topology)
