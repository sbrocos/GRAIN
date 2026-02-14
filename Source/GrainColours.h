/*
  ==============================================================================

    GrainColours.h
    GRAIN — Shared color palette for all UI components.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
namespace GrainColours
{
inline const juce::Colour kBackground{0xff1c1917};   // Dark stone
inline const juce::Colour kSurface{0xff292524};      // Slightly lighter
inline const juce::Colour kText{0xffa8a29e};         // Stone 400
inline const juce::Colour kTextBright{0xfff5f5f4};   // Stone 100
inline const juce::Colour kAccent{0xffd97706};       // Amber 600
inline const juce::Colour kAccentDim{0xff92400e};    // Amber 800
inline const juce::Colour kMeterGreen{0xff22c55e};   // Green 500
inline const juce::Colour kMeterYellow{0xffeab308};  // Yellow 500
inline const juce::Colour kMeterRed{0xffef4444};     // Red 500
}  // namespace GrainColours
