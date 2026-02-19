/*
  ==============================================================================

    PluginEditor.h
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor: layout orchestrator composing UI sub-components.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include "Standalone/AudioFileUtils.h"
#include "UI/Components/BypassControl.h"
#include "UI/Components/ControlPanel.h"
#include "UI/Components/DragDropOverlay.h"
#include "UI/Components/FooterPanel.h"
#include "UI/Components/HeaderPanel.h"
#include "UI/Components/MeterPanel.h"
#include "UI/Components/StandalonePanel.h"
#include "UI/GrainLookAndFeel.h"

#include <JuceHeader.h>

//==============================================================================
/**
 * Plugin editor for GRAIN — layout orchestrator.
 *
 * Composes self-contained sub-components:
 *   - HeaderPanel     : title + subtitle typography
 *   - BypassControl   : LED bypass toggle
 *   - ControlPanel    : all parameter knobs + focus selector
 *   - inputMeter      : IN segmented LED meter (left column)
 *   - outputMeter     : OUT segmented LED meter (right column)
 *   - FooterPanel     : version + copyright strip
 *   - StandalonePanel (optional): waveform + transport + export workflow
 *   - DragDropOverlay (optional): drag & drop visual feedback
 *
 * This class is responsible only for layout (resized), meter timer ticks,
 * bypass alpha propagation, and drag & drop event routing.
 */
class GRAINAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , public juce::FileDragAndDropTarget
    , private juce::Timer
{
public:
    explicit GRAINAudioProcessorEditor(GRAINAudioProcessor& p);
    ~GRAINAudioProcessorEditor() override;

    /** @return true if running as standalone application. */
    bool isStandaloneMode() const { return standaloneMode; }

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    //==============================================================================
    void timerCallback() override;

    GrainLookAndFeel grainLookAndFeel;
    GRAINAudioProcessor& processor;

    // Sub-components (always present)
    HeaderPanel headerPanel;
    BypassControl bypassControl;
    ControlPanel controlPanel;
    MeterPanel inputMeter;
    MeterPanel outputMeter;
    FooterPanel footerPanel;

    // Sub-components (standalone mode only — null in plugin mode)
    std::unique_ptr<StandalonePanel> standalonePanel;
    std::unique_ptr<DragDropOverlay> dragDropOverlay;

    bool standaloneMode = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRAINAudioProcessorEditor)
};
