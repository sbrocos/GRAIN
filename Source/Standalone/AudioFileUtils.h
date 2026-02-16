/*
  ==============================================================================

    AudioFileUtils.h
    GRAIN — Utility functions for audio file handling (GT-19).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
namespace AudioFileUtils
{

/** Check if a file path has a supported audio file extension (.wav, .aiff, .aif). */
inline bool isSupportedAudioFile(const juce::String& filePath)
{
    auto extension = juce::File(filePath).getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".aiff" || extension == ".aif";
}

}  // namespace AudioFileUtils
