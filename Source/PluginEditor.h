/*
  ==============================================================================

    PluginEditor.h
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor: functional layout with controls, meters, and basic styling.
    Phase A — structure and connectivity, not final visual polish.

  ==============================================================================
*/

#pragma once

#include "GrainColours.h"
#include "PluginProcessor.h"
#include "Standalone/AudioFileUtils.h"
#include "Standalone/AudioRecorder.h"
#include "Standalone/FilePlayerSource.h"
#include "Standalone/TransportBar.h"
#include "Standalone/WaveformDisplay.h"
#include "UI/GrainLookAndFeel.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Plugin editor for GRAIN (Phase A — functional layout).
 *
 * Layout:
 *   Header:  Title "GRAIN" + Bypass button
 *   Main:    Grain (drive) + Warmth knobs (large), Input/Output meters (L/R)
 *   Footer:  Input Gain + Mix + Focus selector + Output knob (small)
 *
 * Meters use a 30 FPS timer reading atomic levels from the processor.
 */
class GRAINAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , public juce::FileDragAndDropTarget
    , public FilePlayerSource::Listener
    , private juce::Timer
    , private TransportBar::Listener
{
public:
    explicit GRAINAudioProcessorEditor(GRAINAudioProcessor& /*p*/);
    ~GRAINAudioProcessorEditor() override;

    /** @return true if running as standalone application. */
    bool isStandaloneMode() const { return standaloneMode; }

    //==============================================================================
    void paint(juce::Graphics& /*g*/) override;
    void resized() override;

    //==============================================================================
    // FileDragAndDropTarget (GT-19)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    //==============================================================================
    void timerCallback() override;
    static void drawMeter(juce::Graphics& /*g*/, juce::Rectangle<float> area, float levelL, float levelR,
                          const juce::String& label);

    // Setup helpers (reduce constructor boilerplate)
    void setupRotarySlider(juce::Slider& slider, int textBoxWidth, int textBoxHeight, const juce::String& suffix = {});
    void setupLabel(juce::Label& label);

    GrainLookAndFeel grainLookAndFeel;
    GRAINAudioProcessor& processor;

    // Main controls (creative — big knobs)
    juce::Slider grainSlider;
    juce::Slider warmthSlider;

    // Secondary controls (utility — small knobs)
    juce::Slider inputSlider;
    juce::Slider mixSlider;
    juce::ComboBox focusSelector;
    juce::Slider outputSlider;

    // Bypass
    juce::TextButton bypassButton{"BYPASS"};

    // Labels
    juce::Label grainLabel{{}, "GRAIN"};
    juce::Label warmthLabel{{}, "WARM"};
    juce::Label inputLabel{{}, "INPUT"};
    juce::Label mixLabel{{}, "MIX"};
    juce::Label focusLabel{{}, "FOCUS"};
    juce::Label outputLabel{{}, "OUTPUT"};

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> focusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // Meter display values (smoothed via decay)
    float displayInputL = 0.0f, displayInputR = 0.0f;
    float displayOutputL = 0.0f, displayOutputR = 0.0f;

    static constexpr float kMeterDecay = 0.85f;

    //==============================================================================
    // Standalone mode (GT-17)
    bool standaloneMode = false;

    // Standalone-only components (created only in standalone mode)
    std::unique_ptr<FilePlayerSource> filePlayer;
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<AudioRecorder> recorder;

    // TransportBar::Listener callbacks
    void openFileRequested() override;
    void stopRequested() override;
    void exportRequested() override;

    // FilePlayerSource::Listener callbacks (for export workflow)
    void transportStateChanged(bool isNowPlaying) override;
    void transportReachedEnd() override;

    // Export state
    bool exporting = false;

    // File chooser (must persist during async dialog)
    std::unique_ptr<juce::FileChooser> fileChooser;

    //==============================================================================
    // Drag & drop state (GT-19)
    bool dragHovering = false;
    bool dragAccepted = false;

    /** Load a file into the player and update all standalone components. */
    void loadFileIntoPlayer(const juce::File& file);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRAINAudioProcessorEditor)
};
