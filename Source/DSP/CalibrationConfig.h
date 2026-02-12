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

struct RMSCalibration
{
    float attackMs = 100.0f;   // Slow attack (ignores transients)
    float releaseMs = 300.0f;  // Even slower release (stability)
};

struct BiasCalibration
{
    float amount = 0.3f;  // Internal bias intensity
    float scale = 0.1f;   // Keeps effect in micro-saturation territory
};

struct WaveshaperCalibration
{
    float driveMin = 0.1f;  // Grain 0% maps to this drive
    float driveMax = 0.4f;  // Grain 100% maps to this drive
};

struct WarmthCalibration
{
    float depth = 0.22f;  // Maximum effect depth (22%) - calibrated via listening tests
};

struct FocusCalibration
{
    float lowShelfFreq = 200.0f;    // Hz
    float highShelfFreq = 4000.0f;  // Hz
    float shelfGainDb = 2.8f;       // dB (max boost/cut)
    float shelfQ = 0.707f;          // Butterworth Q
};

struct DCBlockerCalibration
{
    float cutoffHz = 5.0f;  // DC blocker cutoff frequency
};

//==============================================================================
// Top-level calibration config

struct CalibrationConfig
{
    RMSCalibration rms;
    BiasCalibration bias;
    WaveshaperCalibration waveshaper;
    WarmthCalibration warmth;
    FocusCalibration focus;
    DCBlockerCalibration dcBlocker;
};

// Default calibration — reference values for GRAIN's "safe" character
inline constexpr CalibrationConfig kDefaultCalibration{};

}  // namespace GrainDSP
