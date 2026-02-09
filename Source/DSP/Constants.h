/*
  ==============================================================================

    Constants.h
    All DSP constants for GRAIN saturation processor

  ==============================================================================
*/

#pragma once

namespace GrainDSP
{
//==============================================================================
// Math constants
constexpr float kTwoPi = 6.283185307f;
constexpr float kPi = 3.141592653f;

//==============================================================================
// RMS Detector constants
constexpr float kRmsAttackMs = 100.0f;   // Slow attack (ignores transients)
constexpr float kRmsReleaseMs = 300.0f;  // Even slower release (stability)

//==============================================================================
// Dynamic Bias constants (Task 004)
constexpr float kBiasAmount = 0.3f;         // Internal bias intensity (future: from Grain param)
constexpr float kBiasScale = 0.1f;          // Keeps effect in micro-saturation territory
constexpr float kDCBlockerCutoffHz = 5.0f;  // DC blocker cutoff frequency

//==============================================================================
// Warmth constants (Task 005)
constexpr float kWarmthDepth = 0.22f;  // Maximum effect depth (22%) - calibrated via listening tests

//==============================================================================
// Spectral Focus constants (Task 006)
constexpr float kFocusLowShelfFreq = 200.0f;    // Hz
constexpr float kFocusHighShelfFreq = 4000.0f;   // Hz
constexpr float kFocusShelfGainDb = 3.0f;        // dB (max boost/cut)
constexpr float kFocusShelfQ = 0.707f;           // Butterworth Q

}  // namespace GrainDSP
