/*
  ==============================================================================

    PluginEditor.h
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor: HTML/CSS/JS UI via WebBrowserComponent (Phase B).
    Uses JUCE 8 relay pattern for bidirectional APVTS ↔ Web UI sync.

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
 * Plugin editor for GRAIN (Phase B — WebBrowserComponent UI).
 *
 * Renders the custom HTML/CSS/JS UI inside a WebBrowserComponent,
 * using JUCE 8 relay classes for parameter synchronization:
 *   - WebSliderRelay + WebSliderParameterAttachment for sliders
 *   - WebComboBoxRelay + WebComboBoxParameterAttachment for Focus
 *   - WebToggleButtonRelay + WebToggleButtonParameterAttachment for Bypass
 *
 * VU meter levels are sent via custom emitEventIfBrowserIsVisible().
 * Standalone components (transport, waveform, recorder) remain native JUCE.
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

    /** Serve embedded HTML/CSS/JS resources from BinaryData. */
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    GrainLookAndFeel grainLookAndFeel;  // Still used by standalone native components
    GRAINAudioProcessor& processor;

    //==============================================================================
    // Web UI — Relay pattern (JUCE 8)
    // Declaration order matters: relays → webView → attachments

    // Slider relays (one per APVTS float parameter)
    juce::WebSliderRelay driveSliderRelay{"grainSlider"};
    juce::WebSliderRelay warmthSliderRelay{"warmSlider"};
    juce::WebSliderRelay inputSliderRelay{"inputSlider"};
    juce::WebSliderRelay mixSliderRelay{"mixSlider"};
    juce::WebSliderRelay outputSliderRelay{"outputSlider"};

    // Toggle relay (bypass)
    juce::WebToggleButtonRelay bypassToggleRelay{"bypassToggle"};

    // ComboBox relay (focus)
    juce::WebComboBoxRelay focusComboRelay{"focusCombo"};

    // SinglePageBrowser: prevents navigation away from our UI
    struct SinglePageBrowser : juce::WebBrowserComponent
    {
        using juce::WebBrowserComponent::WebBrowserComponent;
        bool pageAboutToLoad(const juce::String& newURL) override;
    };

    SinglePageBrowser webView;

    // Parameter attachments (sync relays ↔ APVTS)
    juce::WebSliderParameterAttachment driveAttachment;
    juce::WebSliderParameterAttachment warmthAttachment;
    juce::WebSliderParameterAttachment inputAttachment;
    juce::WebSliderParameterAttachment mixAttachment;
    juce::WebSliderParameterAttachment outputAttachment;
    juce::WebToggleButtonParameterAttachment bypassAttachment;
    juce::WebComboBoxParameterAttachment focusAttachment;

    //==============================================================================
    // Meter display values (smoothed via decay, sent to web UI)
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
