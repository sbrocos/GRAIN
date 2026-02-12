/*
  ==============================================================================

    CalibrationConfig.h
    Centralized calibration configuration for GRAIN DSP pipeline (Task 007b).
    All tuning values grouped by module as typed structs.
    Compile-time only — no file I/O, no runtime loading.

  ==============================================================================
*/

#pragma once

namespace GrainDSP
{

//==============================================================================
// Per-module calibration structs

/** Calibration for the RMS envelope follower (RMSDetector).
 *  Controls how quickly the detector reacts to level changes. */
struct RMSCalibration
{
    float attackMs = 100.0f;   ///< Attack time in ms — slow to ignore transients
    float releaseMs = 300.0f;  ///< Release time in ms — slower for stability
};

/** Calibration for the dynamic bias stage (DynamicBias).
 *  Introduces asymmetry proportional to the RMS envelope, generating even harmonics. */
struct BiasCalibration
{
    float amount = 0.3f;  ///< Internal bias intensity (0 = none, 1 = full)
    float scale = 0.1f;   ///< Output scaling — keeps effect in micro-saturation territory
};

/** Calibration for the tanh waveshaper (Waveshaper).
 *  Maps the user-facing Drive knob (0–1) to an internal drive range. */
struct WaveshaperCalibration
{
    float driveMin = 0.1f;  ///< Internal drive when user Drive = 0%
    float driveMax = 0.4f;  ///< Internal drive when user Drive = 100%
};

/** Calibration for the warmth processor (WarmthProcessor).
 *  Controls the maximum depth of even-harmonic asymmetry. */
struct WarmthCalibration
{
    float depth = 0.22f;  ///< Maximum effect depth (0.22 = 22%) — calibrated via listening tests
};

/** Calibration for the spectral focus shelf EQ (SpectralFocus).
 *  Defines corner frequencies and gain for the low/high shelf pair. */
struct FocusCalibration
{
    float lowShelfFreq = 200.0f;    ///< Low shelf corner frequency in Hz
    float highShelfFreq = 4000.0f;  ///< High shelf corner frequency in Hz
    float shelfGainDb = 2.8f;       ///< Maximum shelf boost/cut in dB
    float shelfQ = 0.707f;          ///< Shelf Q factor (0.707 = Butterworth)
};

/** Calibration for the DC blocking filter (DCBlocker).
 *  A first-order high-pass that removes DC offset introduced by the bias stage. */
struct DCBlockerCalibration
{
    float cutoffHz = 5.0f;  ///< High-pass cutoff frequency in Hz
};

//==============================================================================

/** Top-level calibration configuration.
 *  Groups all per-module calibration structs into a single object
 *  that can be passed through the DSP pipeline. */
struct CalibrationConfig
{
    RMSCalibration rms;                ///< RMS envelope follower settings
    BiasCalibration bias;              ///< Dynamic bias settings
    WaveshaperCalibration waveshaper;  ///< Waveshaper drive mapping
    WarmthCalibration warmth;          ///< Warmth processor settings
    FocusCalibration focus;            ///< Spectral focus shelf EQ settings
    DCBlockerCalibration dcBlocker;    ///< DC blocker settings
};

/** Default calibration — reference values for GRAIN's "safe", transparent character. */
inline constexpr CalibrationConfig kDefaultCalibration{};

}  // namespace GrainDSP
