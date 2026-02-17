/*
  ==============================================================================

    PluginEditor.cpp
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor implementation: functional layout (Phase A).

  ==============================================================================
*/

#include "PluginEditor.h"

#include "PluginProcessor.h"

namespace
{
constexpr int kEditorWidth = 500;
constexpr int kEditorHeight = 350;
constexpr int kHeaderHeight = 50;
constexpr int kFooterHeight = 100;
constexpr int kMeterColumnWidth = 60;    // meter + padding
constexpr int kTransportBarHeight = 50;  // standalone transport bar
constexpr int kWaveformHeight = 120;     // standalone waveform display
}  // namespace

//==============================================================================
void GRAINAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, int textBoxWidth, int textBoxHeight,
                                                  const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, textBoxHeight);

    if (suffix.isNotEmpty())
    {
        slider.setTextValueSuffix(suffix);
    }

    addAndMakeVisible(slider);
}

void GRAINAudioProcessorEditor::setupLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, GrainColours::kText);
    addAndMakeVisible(label);
}

//==============================================================================
GRAINAudioProcessorEditor::GRAINAudioProcessorEditor(GRAINAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
    , standaloneMode(p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
{

    auto& apvts = processor.getAPVTS();

    // === Main controls (creative — large knobs) ===
    setupRotarySlider(grainSlider, 60, 20);
    grainAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "drive", grainSlider);
    setupLabel(grainLabel);

    setupRotarySlider(warmthSlider, 60, 20);
    warmthAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "warmth", warmthSlider);
    setupLabel(warmthLabel);

    // === Secondary controls (utility — small knobs) ===
    setupRotarySlider(inputSlider, 50, 18, " dB");
    inputAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "inputGain", inputSlider);
    setupLabel(inputLabel);

    setupRotarySlider(mixSlider, 50, 18);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mix", mixSlider);
    setupLabel(mixLabel);

    // === Focus (selector) ===
    focusSelector.addItem("LOW", 1);
    focusSelector.addItem("MID", 2);
    focusSelector.addItem("HIGH", 3);
    addAndMakeVisible(focusSelector);
    focusAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "focus", focusSelector);
    setupLabel(focusLabel);

    setupRotarySlider(outputSlider, 50, 18, " dB");
    outputAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "output", outputSlider);
    setupLabel(outputLabel);

    // === Bypass ===
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, GrainColours::kAccent);
    bypassButton.setColour(juce::TextButton::textColourOffId, GrainColours::kText);
    bypassButton.setColour(juce::TextButton::textColourOnId, GrainColours::kTextBright);
    addAndMakeVisible(bypassButton);
    bypassAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "bypass", bypassButton);

    // === Standalone: file player + waveform + transport bar + recorder (GT-17–GT-20) ===
    if (standaloneMode)
    {
        filePlayer = std::make_unique<FilePlayerSource>();
        filePlayer->addListener(this);

        waveformDisplay = std::make_unique<WaveformDisplay>(*filePlayer);
        addAndMakeVisible(waveformDisplay.get());

        transportBar = std::make_unique<TransportBar>(*filePlayer);
        transportBar->addListener(this);
        addAndMakeVisible(transportBar.get());

        recorder = std::make_unique<AudioRecorder>();

        // Connect file player, waveform display, and recorder to processor
        processor.setFilePlayerSource(filePlayer.get());
        processor.setWaveformDisplay(waveformDisplay.get());
        processor.setAudioRecorder(recorder.get());
    }

    // Set editor size AFTER all components are created, so resized() can lay them out.
    const int editorHeight = standaloneMode ? (kEditorHeight + kWaveformHeight + kTransportBarHeight) : kEditorHeight;
    setSize(kEditorWidth, editorHeight);

    // Start meter timer (30 FPS)
    startTimerHz(30);
}

GRAINAudioProcessorEditor::~GRAINAudioProcessorEditor()
{
    stopTimer();

    if (standaloneMode)
    {
        // Disconnect from processor before destruction
        processor.setAudioRecorder(nullptr);
        processor.setWaveformDisplay(nullptr);
        processor.setFilePlayerSource(nullptr);

        if (filePlayer != nullptr)
        {
            filePlayer->removeListener(this);
        }

        if (transportBar != nullptr)
        {
            transportBar->removeListener(this);
        }
    }
}

//==============================================================================
void GRAINAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(GrainColours::kBackground);

    // Header area
    const auto headerArea = getLocalBounds().removeFromTop(kHeaderHeight);
    g.setColour(GrainColours::kSurface);
    g.fillRect(headerArea);

    // Title — JUCE 8 Font API
    g.setColour(GrainColours::kTextBright);
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    g.drawText("GRAIN", headerArea.reduced(15, 0), juce::Justification::centredLeft);

    // Meters
    auto bounds = getLocalBounds();
    bounds.removeFromTop(kHeaderHeight);
    if (standaloneMode)
    {
        bounds.removeFromBottom(kTransportBarHeight + kWaveformHeight);
    }
    bounds.removeFromBottom(kFooterHeight);

    const auto inputMeterArea = bounds.removeFromLeft(kMeterColumnWidth).toFloat().reduced(10, 20);
    const auto outputMeterArea = bounds.removeFromRight(kMeterColumnWidth).toFloat().reduced(10, 20);

    drawMeter(g, inputMeterArea, displayInputL, displayInputR, "IN");
    drawMeter(g, outputMeterArea, displayOutputL, displayOutputR, "OUT");

    // Footer separator
    g.setColour(GrainColours::kSurface);
    auto footerBounds = getLocalBounds();
    if (standaloneMode)
    {
        footerBounds.removeFromBottom(kTransportBarHeight + kWaveformHeight);
    }
    g.fillRect(footerBounds.removeFromBottom(kFooterHeight));

    // Drag & drop hover overlay (GT-19)
    if (dragHovering)
    {
        constexpr float kBorderWidth = 3.0f;
        auto borderBounds = getLocalBounds().toFloat().reduced(kBorderWidth * 0.5f);
        g.setColour(dragAccepted ? GrainColours::kAccent : GrainColours::kMeterRed);
        g.drawRoundedRectangle(borderBounds, 4.0f, kBorderWidth);
    }
}

void GRAINAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> area, float levelL, float levelR,
                                          const juce::String& label)
{
    const float labelHeight = 20.0f;
    const auto labelArea = area.removeFromTop(labelHeight);

    g.setColour(GrainColours::kText);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(label, labelArea, juce::Justification::centred);

    auto meterArea = area.reduced(2, 0);
    const auto meterBarWidth = (meterArea.getWidth() / 2.0f) - 2.0f;

    const auto leftArea = meterArea.removeFromLeft(meterBarWidth);
    meterArea.removeFromLeft(4);  // Gap between L/R
    const auto rightArea = meterArea;

    // Meter backgrounds
    g.setColour(juce::Colours::black);
    g.fillRect(leftArea);
    g.fillRect(rightArea);

    // Draw level bars
    auto drawLevel = [&](juce::Rectangle<float> meterRect, float level)
    {
        float const dbLevel = juce::Decibels::gainToDecibels(level, -60.0f);
        float normalized = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        auto levelHeight = meterRect.getHeight() * normalized;
        auto levelRect = meterRect.removeFromBottom(levelHeight);

        // Color: green → yellow → red
        if (normalized < 0.7f)
        {
            g.setColour(GrainColours::kMeterGreen);
        }
        else if (normalized < 0.9f)
        {
            g.setColour(GrainColours::kMeterYellow);
        }
        else
        {
            g.setColour(GrainColours::kMeterRed);
        }

        g.fillRect(levelRect);
    };

    drawLevel(leftArea.reduced(1), levelL);
    drawLevel(rightArea.reduced(1), levelR);
}

//==============================================================================
void GRAINAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Standalone: transport bar + waveform at the bottom
    if (standaloneMode)
    {
        if (transportBar != nullptr)
        {
            transportBar->setBounds(bounds.removeFromBottom(kTransportBarHeight));
        }
        if (waveformDisplay != nullptr)
        {
            waveformDisplay->setBounds(bounds.removeFromBottom(kWaveformHeight));
        }
    }

    // Header
    auto headerArea = bounds.removeFromTop(kHeaderHeight);
    bypassButton.setBounds(headerArea.removeFromRight(80).reduced(10));

    // Footer (secondary controls — 4 items: INPUT | MIX | FOCUS | OUTPUT)
    auto footerArea = bounds.removeFromBottom(kFooterHeight);
    auto footerControls = footerArea.reduced(20, 10);

    auto controlWidth = footerControls.getWidth() / 4;

    auto inputArea = footerControls.removeFromLeft(controlWidth);
    inputLabel.setBounds(inputArea.removeFromTop(20));
    inputSlider.setBounds(inputArea);

    auto mixArea = footerControls.removeFromLeft(controlWidth);
    mixLabel.setBounds(mixArea.removeFromTop(20));
    mixSlider.setBounds(mixArea);

    auto focusArea = footerControls.removeFromLeft(controlWidth);
    focusLabel.setBounds(focusArea.removeFromTop(20));
    focusSelector.setBounds(focusArea.reduced(10, 20));

    auto outputArea = footerControls;
    outputLabel.setBounds(outputArea.removeFromTop(20));
    outputSlider.setBounds(outputArea);

    // Main area (big knobs — creative controls: GRAIN + WARMTH)
    auto mainArea = bounds;
    mainArea.removeFromLeft(kMeterColumnWidth);
    mainArea.removeFromRight(kMeterColumnWidth);
    mainArea.reduce(10, 20);

    auto knobWidth = mainArea.getWidth() / 2;

    auto grainArea = mainArea.removeFromLeft(knobWidth);
    grainLabel.setBounds(grainArea.removeFromTop(25));
    grainSlider.setBounds(grainArea);

    auto warmthArea = mainArea;
    warmthLabel.setBounds(warmthArea.removeFromTop(25));
    warmthSlider.setBounds(warmthArea);
}

//==============================================================================
//==============================================================================
// Standalone transport bar callbacks (GT-17)
void GRAINAudioProcessorEditor::openFileRequested()
{
    if (filePlayer == nullptr)
    {
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser>("Open Audio File", juce::File(), "*.wav;*.aiff;*.aif");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto result = fc.getResult();

                                 if (result.existsAsFile())
                                 {
                                     loadFileIntoPlayer(result);
                                 }
                             });
}

void GRAINAudioProcessorEditor::stopRequested()
{
    if (waveformDisplay != nullptr)
    {
        waveformDisplay->clearWetBuffer();
    }
}

void GRAINAudioProcessorEditor::exportRequested()
{
    if (filePlayer == nullptr || !filePlayer->isFileLoaded() || recorder == nullptr)
    {
        return;
    }

    // Build a default filename from the loaded file
    auto loadedFile = filePlayer->getLoadedFile();
    auto defaultName = loadedFile.getFileNameWithoutExtension() + "_processed.wav";
    auto defaultDir = loadedFile.getParentDirectory();

    fileChooser =
        std::make_unique<juce::FileChooser>("Export Processed Audio", defaultDir.getChildFile(defaultName), "*.wav");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                        juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(chooserFlags,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto result = fc.getResult();

                                 if (result == juce::File())
                                 {
                                     return;  // User cancelled
                                 }

                                 // Ensure .wav extension
                                 auto outputFile = result;
                                 if (outputFile.getFileExtension().toLowerCase() != ".wav")
                                 {
                                     outputFile = outputFile.withFileExtension(".wav");
                                 }

                                 // Start recording
                                 auto const sampleRate = filePlayer->getFileSampleRate();
                                 auto const numChannels = filePlayer->getFileNumChannels();

                                 if (!recorder->startRecording(outputFile, sampleRate, numChannels))
                                 {
                                     return;  // Failed to create output file
                                 }

                                 exporting = true;

                                 // Rewind and play the full file
                                 filePlayer->seekToPosition(0.0);
                                 processor.resetPipelines();

                                 if (waveformDisplay != nullptr)
                                 {
                                     waveformDisplay->clearWetBuffer();
                                 }

                                 filePlayer->play();

                                 if (transportBar != nullptr)
                                 {
                                     transportBar->updateButtonStates();
                                 }
                             });
}

//==============================================================================
// FilePlayerSource::Listener callbacks (GT-20 export workflow)

void GRAINAudioProcessorEditor::transportStateChanged(bool /*isNowPlaying*/)
{
    if (transportBar != nullptr)
    {
        transportBar->updateButtonStates();
    }
}

void GRAINAudioProcessorEditor::transportReachedEnd()
{
    if (exporting && recorder != nullptr)
    {
        // Export complete — stop recording and finalize file
        recorder->stopRecording();
        exporting = false;

        if (transportBar != nullptr)
        {
            transportBar->updateButtonStates();
        }
    }
}

//==============================================================================
// Drag & drop (GT-19)

bool GRAINAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (!standaloneMode)
    {
        return false;
    }

    // Accept if at least one file has a supported extension
    return std::any_of(files.begin(), files.end(),
                       [](const juce::String& f) { return AudioFileUtils::isSupportedAudioFile(f); });
}

void GRAINAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    dragHovering = true;
    dragAccepted = std::any_of(files.begin(), files.end(),
                               [](const juce::String& f) { return AudioFileUtils::isSupportedAudioFile(f); });
    repaint();
}

void GRAINAudioProcessorEditor::fileDragExit(const juce::StringArray& /*files*/)
{
    dragHovering = false;
    dragAccepted = false;
    repaint();
}

void GRAINAudioProcessorEditor::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    dragHovering = false;
    dragAccepted = false;
    repaint();

    if (!standaloneMode || filePlayer == nullptr)
    {
        return;
    }

    // Find the first supported audio file
    for (const auto& filePath : files)
    {
        if (AudioFileUtils::isSupportedAudioFile(filePath))
        {

            if (const juce::File audioFile(filePath); audioFile.existsAsFile())
            {
                loadFileIntoPlayer(audioFile);
                return;
            }
        }
    }
}

void GRAINAudioProcessorEditor::loadFileIntoPlayer(const juce::File& file)
{
    if (filePlayer == nullptr)
    {
        return;
    }

    filePlayer->loadFile(file);
    filePlayer->prepareToPlay(processor.getSampleRate(), processor.getBlockSize());

    if (transportBar != nullptr)
    {
        transportBar->updateButtonStates();
    }

    if (waveformDisplay != nullptr)
    {
        waveformDisplay->clearWetBuffer();
    }
}

//==============================================================================
void GRAINAudioProcessorEditor::timerCallback()
{
    // Smooth meter decay
    // NOTE: Phase A repaints entire editor at 30 FPS for simplicity.
    // Phase B should optimize to repaint only meter areas.
    float const inL = processor.inputLevelL.load();
    float const inR = processor.inputLevelR.load();
    float const outL = processor.outputLevelL.load();
    float const outR = processor.outputLevelR.load();

    displayInputL = std::max(inL, displayInputL * kMeterDecay);
    displayInputR = std::max(inR, displayInputR * kMeterDecay);
    displayOutputL = std::max(outL, displayOutputL * kMeterDecay);
    displayOutputR = std::max(outR, displayOutputR * kMeterDecay);

    repaint();
}
