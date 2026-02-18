/*
  ==============================================================================

    GrainColours.h
    GRAIN — Shared color palette for all UI components.
    Based on the approved JSX mockup (sage green theme).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
namespace GrainColours
{
// === Background & Surfaces ===
inline const juce::Colour kBackground{0xffB6C1B9};    // Sage green (main bg)
inline const juce::Colour kSurface{0xff1a1a1a};       // Knob body, meters bg
inline const juce::Colour kSurfaceLight{0xff3a3a3a};  // Knob ring, inactive dots

// === Accent ===
inline const juce::Colour kAccent{0xffd97706};  // Active dots, selected buttons

// === Text ===
inline const juce::Colour kText{0xff1a1a1a};        // Labels on light bg
inline const juce::Colour kTextDim{0xff4a4a4a};     // Subtitle, footer
inline const juce::Colour kTextBright{0xffffffff};  // Text on dark bg
inline const juce::Colour kTextMuted{0xff888888};   // Inactive button text

// === Meters ===
inline const juce::Colour kMeterGreen{0xff22c55e};   // 0-75%
inline const juce::Colour kMeterYellow{0xffeab308};  // 75-90%
inline const juce::Colour kMeterRed{0xffef4444};     // 90-100%
inline const juce::Colour kMeterOff{0xff3a3a3a};     // Inactive segments

// === Bypass LED ===
inline const juce::Colour kBypassOn{0xffd97706};   // Processing (orange)
inline const juce::Colour kBypassOff{0xff1a1a1a};  // Bypassed (dark)
}  // namespace GrainColours
