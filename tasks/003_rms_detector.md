# Task 003: RMS Detector (Slow Envelope Follower)

## Objective

Implement a slow RMS level detector that provides a stable envelope for the Dynamic Bias stage. This detector intentionally does NOT chase transients — it captures the overall energy/texture of the signal.

```
Input → [RMS Detector] → envelope value → (used by Dynamic Bias in Task 004)
```

---

## Prerequisites

- Task 002 completed (MVP DSP working)

---

## Design Rationale

### Why Slow RMS?

| Fast Detector | Slow RMS (our choice) |
|---------------|----------------------|
| Follows transients | Ignores transients |
| Pumping/breathing artifacts | Stable, textural behavior |
| Compressor-like | Saturation character |

### Time Constants

```cpp
// En GrainDSP.h como constexpr (mejor mantenibilidad)
namespace GrainDSP
{
    constexpr float RMS_ATTACK_MS = 100.0f;   // Slow attack
    constexpr float RMS_RELEASE_MS = 300.0f;  // Even slower release
}
```

**Decisión:** Constantes como `constexpr` en el header para mejor mantenibilidad.

These values are intentionally long to avoid transient-chasing.

---

## Architecture

### Pure Function (in `Source/DSP/GrainDSP.h`)

```cpp
namespace GrainDSP
{
    // RMS Detector state (needs to persist between samples)
    struct RMSDetector
    {
        float envelope = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        
        void prepare(float sampleRate, float attackMs, float releaseMs)
        {
            // One-pole filter coefficients
            attackCoeff = std::exp(-1.0f / (sampleRate * attackMs * 0.001f));
            releaseCoeff = std::exp(-1.0f / (sampleRate * releaseMs * 0.001f));
        }
        
        void reset()
        {
            envelope = 0.0f;
        }
        
        float process(float input)
        {
            float inputSquared = input * input;
            
            // Asymmetric ballistics: different attack/release
            float coeff = (inputSquared > envelope) ? attackCoeff : releaseCoeff;
            
            // One-pole smoothing filter
            envelope = envelope * coeff + inputSquared * (1.0f - coeff);
            
            // Return RMS (square root of mean square)
            return std::sqrt(envelope);
        }
    };
    
    // Stateless helper for testing coefficient calculation
    inline float calculateCoefficient(float sampleRate, float timeMs)
    {
        return std::exp(-1.0f / (sampleRate * timeMs * 0.001f));
    }
}
```

### Integration in PluginProcessor

```cpp
// In PluginProcessor.h
GrainDSP::RMSDetector rmsDetector;
float currentEnvelope = 0.0f;  // Guardar para uso futuro (Task 004)

// In prepareToPlay() - llamar prepare() Y reset()
rmsDetector.prepare(static_cast<float>(sampleRate),
                    GrainDSP::RMS_ATTACK_MS,
                    GrainDSP::RMS_RELEASE_MS);
rmsDetector.reset();  // Limpiar historial del filtro

// In processBlock() - UNA VEZ por sample-frame, no por canal
for (int sample = 0; sample < numSamples; ++sample)
{
    float L = leftChannel[sample];
    float R = rightChannel[sample];
    float mono = (L + R) * 0.5f;
    currentEnvelope = rmsDetector.process(mono);  // Una vez por frame

    // ... resto del procesamiento por canal
}
```

**Decisiones:**
- Guardar envelope en variable miembro para preparar Task 004
- Llamar `prepare()` Y `reset()` en prepareToPlay() (prepare configura coeficientes, reset limpia historial)
- Procesar detector una vez por sample-frame, no por canal
- **Integrar ya en processBlock** aunque el envelope no se use todavía (evita sorpresas de integración en Task 004)

---

## Stereo Handling

For GRAIN's "linked stereo" design (from PRD), use a single RMS detector fed by the sum of both channels:

```cpp
// Sum (more accurate RMS) - DECISIÓN FINAL
float monoInput = (leftSample + rightSample) * 0.5f;
float envelope = rmsDetector.process(monoInput);
```

**Decisión:** Usar suma `(L + R) * 0.5f` para un comportamiento más suave y predecible.

---

## Unit Tests

Add to `Source/Tests/DSPTests.cpp`:

```cpp
void runRMSDetectorTests()
{
    beginTest("RMS Detector: coefficient calculation");
    {
        float sampleRate = 44100.0f;
        float timeMs = 100.0f;
        float coeff = GrainDSP::calculateCoefficient(sampleRate, timeMs);
        
        // Coefficient should be between 0 and 1
        expect(coeff > 0.0f);
        expect(coeff < 1.0f);
        
        // Longer time = higher coefficient (slower response)
        float coeffSlow = GrainDSP::calculateCoefficient(sampleRate, 200.0f);
        expect(coeffSlow > coeff);
    }

    beginTest("RMS Detector: zero input produces zero output");
    {
        GrainDSP::RMSDetector detector;
        detector.prepare(44100.0f, 100.0f, 300.0f);
        
        for (int i = 0; i < 1000; ++i)
        {
            detector.process(0.0f);
        }
        
        float result = detector.process(0.0f);
        expectWithinAbsoluteError(result, 0.0f, TestConstants::TOLERANCE);
    }

    beginTest("RMS Detector: DC input converges to that value");
    {
        GrainDSP::RMSDetector detector;
        detector.prepare(44100.0f, 100.0f, 300.0f);

        float constantInput = 0.5f;
        float result = 0.0f;

        // Process enough samples to converge (10x attack time)
        int samplesToConverge = static_cast<int>(44100.0f * 1.0f);  // 1 second
        for (int i = 0; i < samplesToConverge; ++i)
        {
            result = detector.process(constantInput);
        }

        // Should converge to input value (RMS of constant = constant)
        expectWithinAbsoluteError(result, constantInput, 0.01f);
    }

    beginTest("RMS Detector: sine wave converges to RMS value");
    {
        GrainDSP::RMSDetector detector;
        detector.prepare(44100.0f, 100.0f, 300.0f);

        float amplitude = 1.0f;
        float frequency = 440.0f;
        float sampleRate = 44100.0f;
        float result = 0.0f;

        // Process enough samples to converge (1 second)
        int samplesToConverge = static_cast<int>(sampleRate * 1.0f);
        for (int i = 0; i < samplesToConverge; ++i)
        {
            float phase = 2.0f * 3.14159265f * frequency * i / sampleRate;
            float sample = amplitude * std::sin(phase);
            result = detector.process(sample);
        }

        // RMS of sine wave = amplitude / sqrt(2) ≈ 0.707
        float expectedRMS = amplitude / std::sqrt(2.0f);
        expectWithinAbsoluteError(result, expectedRMS, 0.01f);
    }

    beginTest("RMS Detector: envelope is always non-negative");
    {
        GrainDSP::RMSDetector detector;
        detector.prepare(44100.0f, 100.0f, 300.0f);
        
        // Test with negative input
        for (int i = 0; i < 100; ++i)
        {
            float result = detector.process(-0.5f);
            expect(result >= 0.0f);
        }
    }

    beginTest("RMS Detector: slow response to transients");
    {
        GrainDSP::RMSDetector detector;
        detector.prepare(44100.0f, 100.0f, 300.0f);
        
        // Start with silence
        for (int i = 0; i < 100; ++i)
        {
            detector.process(0.0f);
        }
        
        // Hit with sudden transient
        float immediateResponse = detector.process(1.0f);
        
        // Should NOT immediately jump to 1.0 (that would be fast/peak detection)
        expect(immediateResponse < 0.5f);
    }

    beginTest("RMS Detector: reset clears state");
    {
        GrainDSP::RMSDetector detector;
        detector.prepare(44100.0f, 100.0f, 300.0f);
        
        // Build up envelope
        for (int i = 0; i < 1000; ++i)
        {
            detector.process(1.0f);
        }
        
        detector.reset();
        
        // After reset, envelope should be zero
        float result = detector.process(0.0f);
        expectWithinAbsoluteError(result, 0.0f, TestConstants::TOLERANCE);
    }
}
```

Don't forget to call `runRMSDetectorTests()` from `runTest()`.

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Source/DSP/GrainDSP.h` | Add RMSDetector struct |
| `Source/PluginProcessor.h` | Add RMSDetector member |
| `Source/PluginProcessor.cpp` | Initialize and call in processBlock |
| `Source/Tests/DSPTests.cpp` | Add RMS detector tests |

---

## Acceptance Criteria

### Functional
- [ ] RMS detector compiles without errors
- [ ] Detector responds slowly to transients (no pumping)
- [ ] Constant input converges to that value
- [ ] Zero input produces zero output
- [ ] Reset clears state properly

### Quality
- [ ] pluginval still passes
- [ ] All previous tests still pass
- [ ] New RMS tests pass

### Integration
- [ ] Detector runs in processBlock (even if output not yet used)
- [ ] Linked stereo: single detector for both channels

---

## Testing Checklist

```bash
# 1. Build
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# 2. Run pluginval
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3

# 3. Check unit tests (console output)

# 4. Manual verification
# - Load plugin on drum bus
# - Observe: envelope should NOT pump with kick/snare
# - Should feel "stable" and "textural"
```

---

## Technical Notes

### One-Pole Filter

The RMS detector uses a simple one-pole IIR filter:

```
y[n] = coeff * y[n-1] + (1 - coeff) * x[n]
```

Where `coeff` determines the time constant. Higher coeff = slower response.

### Asymmetric Ballistics

Attack and release have different coefficients:
- **Attack** (signal rising): slightly faster to catch level increases
- **Release** (signal falling): slower to maintain stability

### Why Not Use JUCE's EnvelopeFollower?

JUCE doesn't have a built-in RMS detector with these specific characteristics. Our implementation is simple, testable, and tailored to GRAIN's needs.

---

## Out of Scope (future tasks)

- Dynamic Bias using this envelope (Task 004)
- Exposing attack/release as parameters
- Separate L/R detection mode
