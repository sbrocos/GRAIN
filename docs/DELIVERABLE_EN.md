# ACADEMIC DELIVERABLE — FINAL PROJECT  
## GRAIN: Micro-harmonic saturation processor (VST3 + AU + Standalone)

**Author:** Sergio Brocos
**Program:** Master Desarrollo con IA
**Date:** [Date]  
**Repository:** [GitHub URL]  
**Release/Build:** [Release URL or build link]

---

## 1. Abstract
GRAIN is a micro-harmonic saturation processor designed to enrich digital signals in a subtle, controlled, and stable way. Its goal is to increase perceived density and coherence without introducing obvious distortion or altering transients, tonal balance, or stereo image—especially within typical usage ranges (10–20% wet). The system is delivered as a VST3 and AU plugin for macOS (Apple Silicon and Intel) and as a minimal real-time standalone monitoring application. The DSP design relies on a smooth nonlinear transfer function (tanh), level-dependent mild asymmetry (slow RMS-based behavior), and a band-focused spectral emphasis (Low/Mid/High). Oversampling is managed internally (2× in real-time and 4× during offline rendering) to reduce aliasing while keeping the user experience simple.

---

## 2. Background and motivation

### 2.1 Personal motivation

GRAIN sits at the intersection of two paths that have defined my life in parallel: software development and music production.

As a Senior Ruby on Rails developer, I have spent years building robust systems and thinking in abstractions. Music production, however, has always been my creative counterweight — something I pursued with the same intensity but in a completely different dimension. For a long time, these two worlds coexisted separately. I knew how to write code, and I knew how to produce music, but building an audio plugin — something that lives inside a DAW and processes sound in real time — felt like a completely different discipline, one I didn’t know how to approach.

AI changed that. Working with AI as a development partner allowed me to navigate an unfamiliar domain (C++, JUCE, DSP) while applying the engineering mindset I already had. It didn’t write the project for me — it gave me the map to find my own way through it.

GRAIN is the result of that convergence: a tool I actually use in my own productions, built with skills I acquired through this master’s program, in a domain I was passionate about but never thought I could access.

### 2.2 Technical motivation

Digital production workflows may sound “cold”, “flat”, or lacking glue on certain buses (drums, pads, mix bus). Many saturation tools add strong coloration, which can be intrusive for mixing or light pre-master use. GRAIN is conceived as a “safe” processor: its contribution should be mostly perceived when bypassed (bypass reveal), providing density and stability without a strong audible color.

---

## 3. Objectives
### 3.1 General objective
Design and implement a transparent micro-harmonic saturation processor for bus and mix/pre-master use, with stable behavior and no perceptible artifacts.

### 3.2 Specific objectives
1. Define a PRD/PDR focused on safety, stability, and a simple UX.  
2. Implement a DSP chain based on:
   - slow RMS detection,
   - controlled triode-like asymmetry,
   - smooth tanh waveshaping,
   - discrete spectral focus control (Low/Mid/High).
3. Reduce aliasing via internal oversampling not exposed to the user.
4. Ensure zero-latency in real-time processing.
5. Deliver both VST3 and AU plugins and a minimal standalone application.
6. Validate through listening and technical checks (bypass reveal, stability, no level jumps).

---

## 4. Scope and deliverables
### 4.1 Scope (V1)
- **VST3 and AU** plugins for **macOS (Apple Silicon ARM64 + Intel x86_64)**.
- Minimum guaranteed sample rate: **44.1 kHz**.
- User parameters: Input Gain, Drive (Grain), Warmth, Focus (Low/Mid/High), Mix, Output Gain, Bypass.
- Internal oversampling: 2× real-time; 4× offline render.
- Stereo: linked processing (shared detector).
- Latency: 0 samples in real-time.
- Standalone: real-time monitoring with device selector, bypass, and meters.

### 4.2 Out of scope (V1)
- Presets and A/B (planned for V1.1).
- Exposed technical parameters (oversampling, internal ceilings).

### 4.3 Deliverables
- Final PRD/PDR document (Markdown).
- Source code and documentation (README, build steps).
- Binaries or reproducible build instructions (plugin + standalone).
- Validation plan and results (listening + basic technical tests).

---

## 5. Requirements (functional and non-functional)
### 5.1 Functional requirements
- Subtle micro-harmonic enhancement with intensity control (Grain).
- Very subtle Warmth control: slight even/odd balance adjustment with no obvious A/B change.
- Discrete spectral focus (Low/Mid/High).
- Wet/dry mix and manual final level via Output Gain.

### 5.2 Non-functional requirements
- **Transparency at 10–20% wet:** no perceptible changes to transients, tonal balance, or stereo image.
- **Stability:** no artifacts, no instability on sustained material, no level jumps.
- **Performance:** reasonable CPU usage across multiple instances (moderate oversampling).
- **Usability:** simple interface and no technical decisions required from the user.
- **Compatibility:** macOS Apple Silicon (ARM64) and Intel (x86_64); minimum 44.1 kHz.
- **Latency:** zero in real-time.

---

## 6. Technical design (DSP)
### 6.1 Pipeline overview
1) Input Gain (-12 to +12 dB)
2) Upsample (2× real-time, 4× offline)
3) Slow RMS level detector (shared mono-summed envelope)
4) Dynamic Bias (mild asymmetry driven by RMS)
5) Micro waveshaper (tanh)
6) Harmonic weighting / Warmth (very subtle)
7) Spectral Focus (Low/Mid/High)
8) Downsample
9) Mix (dry/wet)
10) DC Blocker
11) Output Gain

### 6.2 Rationale for DSP choices
- **tanh**: progressive transition into nonlinearity, suitable for micro-saturation and stable spectral behavior.
- **Slow RMS detector**: avoids transient chasing; behavior becomes textural and stable.
- **Mild triode-like asymmetry**: introduces even harmonics without strong coloration; minimal offset and cumulative effect.
- **Internal oversampling (2×/4×)**: reduces aliasing without excessive CPU cost and without exposing technical controls.
- **Stereo link**: reduces the risk of unwanted stereo image shifts on the mix bus.

---

## 7. Methodology and development process
- Iterative workflow: requirements → DSP prototype → listening calibration → optimization → final validation.
- AI assistance: used to support PRD writing, architecture review, test checklist generation, and documentation consistency.  
  *Note:* final design decisions and listening validation remain the author’s responsibility.

---

## 8. Implementation plan (high level)
1. VST3 + AU plugin skeleton (stereo processing, parameters, state).
2. Core DSP (tanh + drive + mix).
3. Integrate slow RMS detector and dynamic bias.
4. Implement Focus (3 bands) with safe transitions.
5. Add internal oversampling (2× real-time; 4× offline).
6. Level stability checks (no auto-gain; manual output).
7. Minimal standalone app: device IO + passthrough + controls + meters.
8. Listening/technical validation and final documentation.

---

## 9. Validation and testing
### 9.1 Listening validation
- At 10–20% wet:
  - no perceived distortion,
  - no changes to transients/tonality/stereo,
  - bypass reveal: reduced density/stability when disabled.

### 9.2 Basic technical checks
- No level jumps when toggling bypass.
- Sustained material (pads/mix): no wobble/instability.
- Basic aliasing review (with/without oversampling comparison).
- CPU: qualitative check with multiple instances.

---

## 10. Risks and mitigations
- **Audible aliasing:** mitigated by 2× real-time and 4× offline oversampling.
- **Unwanted stereo shifts:** mitigated by linked stereo detection and conservative design.
- **Over-coloration when driven:** mitigated by mild asymmetry, smooth curves, and safe defaults.
- **High CPU usage:** mitigated by moderate oversampling and no extreme modes.
- **Too subtle perception:** mitigated by allowing controlled pushing while preserving the “safe” character.

---

## 11. Ethics and quality considerations
- Avoids misleading behavior (e.g., hidden auto-gain that biases perception).
- AI is used as a support tool with clear authorship responsibility.
- Prioritizes user safety and predictability (no unstable or surprising behavior).

---

## 12. Roadmap (V1.1)
- A/B
- Presets
- Optional stereo Link/Unlink (if validated by real use cases)

---

## 13. Conclusion
GRAIN proposes a stability-first approach to micro-harmonic saturation for buses and light pre-master use. The combination of a slow RMS detector, controlled triode-like asymmetry, a tanh-based waveshaper, and moderate internal oversampling aims to deliver subtle density and cohesion without artifacts or intrusive tonal/stereo changes. The deliverables include VST3 and AU plugins for macOS (Apple Silicon and Intel) and a minimal standalone application for testing and basic usage.

---
