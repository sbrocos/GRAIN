# GRAIN — Micro-harmonic Saturation Processor

> *"The kind of saturation you feel when bypassed, not when engaged."*

GRAIN is a subtle micro-harmonic saturation processor designed for bus and mix processing. It adds density and cohesion without obvious distortion, preserving transients, tonal balance, and stereo image — especially within typical usage ranges (10–20% wet).

---

## What is GRAIN?

Digital production workflows can sometimes sound "cold", "flat", or lacking glue — particularly on drum buses, pad layers, or the mix bus. Most saturation tools solve this by adding strong coloration, which can be too intrusive for mixing or light pre-master use.

GRAIN takes a different approach: it's designed to be a **"safe" processor**. Its contribution is mostly perceived when you bypass it (bypass reveal) — providing density, warmth, and cohesion without a strong audible color.

### Under the hood

The DSP pipeline is built around a combination of:

- **tanh waveshaper** — smooth, progressive nonlinearity that generates micro-harmonics without harsh clipping characteristics
- **Slow RMS detector** — a level-dependent envelope (150ms attack / 300ms release by default) that drives mild asymmetry without chasing transients
- **Dynamic Bias** — a quadratic even-harmonic injection tied to the RMS envelope, creating a subtle triode-like character
- **Warmth** — a half-wave blend that shifts even/odd harmonic balance, capped at 10% depth to remain non-invasive
- **Spectral Focus** — biquad shelf EQ that pre-emphasizes the selected band before saturation, shaping where harmonics are generated
- **Internal oversampling** — 2× in real-time, 4× during offline render — transparent to the user, reduces aliasing from the nonlinear stages
- **DC Blocker** — ensures no DC offset accumulation after asymmetric processing
- **Stereo link** — both channels share a single mono-summed RMS detector, preventing unwanted stereo image shifts

Zero latency in real-time. No technical decisions required from the user.

---

## Formats & Compatibility

| Format | Status |
|--------|--------|
| VST3 | ✅ macOS Apple Silicon (ARM64) |
| AU (Audio Unit) | ✅ macOS Apple Silicon (ARM64) |
| Standalone App | ✅ Real-time monitoring with device selector |

**Minimum system:** macOS 11.0+, Apple Silicon (ARM64), 44.1 kHz sample rate.

---

## Installation

### VST3 Plugin

```bash
cp -r GRAIN.vst3 ~/Library/Audio/Plug-Ins/VST3/
# or system-wide:
cp -r GRAIN.vst3 /Library/Audio/Plug-Ins/VST3/
```

### AU Plugin (Audio Unit)

```bash
cp -r GRAIN.component ~/Library/Audio/Plug-Ins/Components/
# or system-wide:
cp -r GRAIN.component /Library/Audio/Plug-Ins/Components/
```

After installing, rescan plugins in your DAW:
- **Logic Pro:** Close and reopen
- **Ableton Live:** Preferences → Plug-Ins → Rescan
- **REAPER:** Options → Preferences → Plug-ins → Re-scan

### Standalone App

Copy `GRAIN.app` to `/Applications/` or run from any location.

> **Note:** On first launch, macOS will request microphone permission for audio input. This is required for real-time processing.

---

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Input Gain** | -12 to +12 dB | 0 dB | Pre-saturation level trim. Use to drive harder into the nonlinear stages. |
| **Grain (Drive)** | 0–100% | 50% | Saturation intensity. Controls pre-tanh gain (1× to 4×). Higher = more harmonics. |
| **Warmth** | 0–100% | 0% | Even/odd harmonic balance via half-wave blend. Very subtle by design (max 10% depth). |
| **Focus** | Low / Mid / High | Mid | Spectral emphasis before saturation. Shapes where harmonics are generated. |
| **Mix** | 0–100% | 20% | Dry/wet blend. Bypass is implemented via smooth mix transition to avoid clicks. |
| **Output Gain** | -12 to +12 dB | 0 dB | Post-processing level trim. No auto-gain is applied. |
| **Bypass** | On/Off | Off | Smooth bypass via mix smoothing — no level jumps. |

---

## Typical Usage

GRAIN is designed to be used conservatively. Less is more.

**Mix bus**
Start at 10–15% wet, Focus: Mid, Grain at 40–50%. The goal is the bypass reveal: when you bypass and the mix sounds slightly thinner, you've found the sweet spot.

**Drum bus**
20–40% wet, Focus: Low or Mid. Adds density and cohesion between kick and snare without smearing transients.

**Pad / Synth bus**
10–20% wet, Focus: Mid or High. Adds subtle harmonic richness that helps synths sit in dense arrangements.

**Vocals**
5–15% wet, Focus: Mid. Light saturation that adds presence and weight without obvious distortion.

**Master bus**
5–10% wet. Approach with caution. At this stage small changes have big consequences — stay in the 5–10% range and validate on mono.

---

## Building from Source

GRAIN is open source under GPL-3.0. The project uses JUCE 8.x with a standard Projucer workflow.

### Prerequisites

- macOS 11.0+ (Apple Silicon, ARM64)
- Xcode 16+
- JUCE 8.x (Projucer) — [download](https://juce.com/download/)
- Command Line Tools: `xcode-select --install`

### Setup

```bash
# Clone the repository
git clone https://github.com/[username]/GRAIN.git
cd GRAIN

# Open the project in Projucer
open GRAIN.jucer

# From Projucer: Save and Open in IDE (Xcode)
# Build once in Xcode (Cmd+B) to generate the compilation database
```

### Development Build

```bash
# Build all targets + run pluginval validation + open DAW
./bin/build -a

# Build VST3 only
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" \
  -configuration Debug build

# Plugin output path (auto-installed):
# ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

### Run Tests

```bash
# Unit tests (per DSP module) + integration tests (full pipeline) + auval
./bin/run_tests
```

### Release Build

```bash
# Release: AU + VST3 + Standalone (optimized, 4× offline oversampling enabled)
./bin/build -R -u -v -s
```

### Project Structure

```
GRAIN/
├── Source/
│   ├── PluginProcessor.{h,cpp}   # Main audio processor (APVTS, oversampling)
│   ├── PluginEditor.{h,cpp}      # GUI (functional layout, GrainColours)
│   └── DSP/
│       ├── CalibrationConfig.h   # Centralized calibration constants
│       ├── RMSDetector.h         # Slow RMS envelope follower (stateful)
│       ├── DynamicBias.h         # Level-dependent asymmetric bias (pure)
│       ├── Waveshaper.h          # tanh waveshaper (pure)
│       ├── WarmthProcessor.h     # Even/odd harmonic shaping (pure)
│       ├── SpectralFocus.h       # Biquad shelf EQ per band (stateful)
│       ├── DCBlocker.h           # DC offset filter (stateful)
│       └── GrainDSPPipeline.h   # Per-channel DSP orchestrator
├── Source/Tests/                 # Unit & integration test suite
├── bin/                          # Build and test scripts
├── GRAIN.jucer                   # Projucer project (VST3 + Standalone + AU)
└── GRAINTests.jucer              # Separate ConsoleApp test runner
```

---

## DSP Pipeline (simplified)

```
Input
  └─ Input Gain
       └─ [Upsample 2× / 4×]
            └─ Mono-sum RMS detector
                 └─ Dynamic Bias (quadratic even-harmonic injection)
                      └─ tanh Waveshaper (pre-gain: 1× to 4×)
                           └─ Warmth (half-wave blend, max 10% depth)
                                └─ Spectral Focus (biquad shelf: Low / Mid / High)
                                     └─ [Downsample]
                                          └─ DC Blocker
                                               └─ Mix (dry/wet, handles bypass)
                                                    └─ Output Gain
                                                         └─ Output
```

---

## Validation

### What to listen for

The primary validation method for GRAIN is the **bypass reveal**: enable the plugin at 10–20% wet and toggle bypass. If the mix sounds slightly thinner, flatter, or less cohesive without GRAIN, it's working correctly. If you can hear a clear tonal change or distortion — reduce the drive or mix.

### Technical checks

- No level jumps on bypass toggle
- No instability on sustained pads or mix bus material
- CPU: < 5% (single instance), < 30% (8 instances) at 44.1 kHz
- Latency: 0 samples reported in real-time; correct latency compensation in offline mode

---

## Roadmap

### V1 (current)
- VST3 + AU plugin for macOS Apple Silicon
- Standalone app with real-time monitoring
- Core DSP chain: tanh + RMS + dynamic bias + warmth + spectral focus + oversampling

### V1.1 (planned)
- A/B comparison
- Parameter presets
- Optional stereo link/unlink
- Optional audio file player in standalone (dry/wet A/B without loopback)

### Future
- Custom UI (Phase B visual design)
- Optional stereo independent processing mode

---

## License

**GPL-3.0** — See [LICENSE](LICENSE) file.

GRAIN uses [JUCE](https://juce.com), distributed under the GPLv3 license for open-source projects.

---

## Author

**Sergio Brocos**  
Master en Desarrollo con IA  
[GitHub](https://github.com/sbrocos) · [Gumroad](https://theichiwave.gumroad.com.gumroad.com)
