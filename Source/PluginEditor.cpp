/*
  ==============================================================================

    PluginEditor.cpp
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor: layout orchestrator composing UI sub-components.

  ==============================================================================
*/

#include "PluginEditor.h"

namespace
{
constexpr int kEditorWidth = 524;
constexpr int kEditorHeight = 617;
constexpr int kPadding = 24;
constexpr int kHeaderHeight = 60;
constexpr int kFooterInfoHeight = 36;
constexpr int kMeterColumnWidth = 100;
constexpr int kTransportBarHeight = 50;  // standalone transport bar
constexpr int kWaveformHeight = 120;     // standalone waveform display
}  // namespace

//==============================================================================
GRAINAudioProcessorEditor::GRAINAudioProcessorEditor(GRAINAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
    , bypassControl(p.getAPVTS())
    , controlPanel(p.getAPVTS())
    , inputMeter(p.inputLevelL, p.inputLevelR, "IN")
    , outputMeter(p.outputLevelL, p.outputLevelR, "OUT")
    , standaloneMode(p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
{
    setLookAndFeel(&grainLookAndFeel);

    addAndMakeVisible(headerPanel);
    addAndMakeVisible(bypassControl);
    addAndMakeVisible(controlPanel);
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(footerPanel);

    if (standaloneMode)
    {
        standalonePanel = std::make_unique<StandalonePanel>(processor);
        addAndMakeVisible(standalonePanel.get());

        dragDropOverlay = std::make_unique<DragDropOverlay>();
        addAndMakeVisible(dragDropOverlay.get());
    }

    const int editorHeight = standaloneMode ? (kEditorHeight + kWaveformHeight + kTransportBarHeight) : kEditorHeight;
    setSize(kEditorWidth, editorHeight);

    startTimerHz(30);
}

GRAINAudioProcessorEditor::~GRAINAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void GRAINAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(GrainColours::kBackground);
}

//==============================================================================
void GRAINAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Standalone: panel at the bottom (outside padding)
    if (standaloneMode && standalonePanel != nullptr)
    {
        standalonePanel->setBounds(bounds.removeFromBottom(kTransportBarHeight + kWaveformHeight));

        // Drag & drop overlay covers the entire editor
        if (dragDropOverlay != nullptr)
        {
            dragDropOverlay->setBounds(getLocalBounds());
        }
    }

    // Apply padding to remaining content area
    auto content = bounds.reduced(kPadding);

    // Header (60px): text panel + bypass button
    auto headerArea = content.removeFromTop(kHeaderHeight);
    headerPanel.setBounds(headerArea.withTrimmedRight(50));
    bypassControl.setBounds(headerArea.getRight() - 40, headerArea.getCentreY() - 20, 40, 40);

    // Footer (36px)
    auto footerArea = content.removeFromBottom(kFooterInfoHeight);
    footerPanel.setBounds(footerArea);

    // Main area: 3-column layout (meter | controls | meter)
    auto leftColumn = content.removeFromLeft(kMeterColumnWidth);
    auto rightColumn = content.removeFromRight(kMeterColumnWidth);

    // Meters in left and right columns; ControlPanel in center
    inputMeter.setBounds(leftColumn.reduced(10, 5));
    outputMeter.setBounds(rightColumn.reduced(10, 5));
    controlPanel.setBounds(content);
}

//==============================================================================
void GRAINAudioProcessorEditor::timerCallback()
{
    inputMeter.updateLevels();
    outputMeter.updateLevels();

    // Bypass alpha: dim all controls to 40% when bypassed
    const float alpha = bypassControl.isBypassed() ? 0.4f : 1.0f;
    controlPanel.setBypassAlpha(alpha);
}

//==============================================================================
// FileDragAndDropTarget

bool GRAINAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (!standaloneMode)
    {
        return false;
    }

    return std::any_of(files.begin(), files.end(),
                       [](const juce::String& f) { return AudioFileUtils::isSupportedAudioFile(f); });
}

void GRAINAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (dragDropOverlay == nullptr)
    {
        return;
    }

    const bool accepted = std::any_of(files.begin(), files.end(),
                                      [](const juce::String& f) { return AudioFileUtils::isSupportedAudioFile(f); });
    dragDropOverlay->setDragState(true, accepted);
}

void GRAINAudioProcessorEditor::fileDragExit(const juce::StringArray& /*files*/)
{
    if (dragDropOverlay != nullptr)
    {
        dragDropOverlay->setDragState(false, false);
    }
}

void GRAINAudioProcessorEditor::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (dragDropOverlay != nullptr)
    {
        dragDropOverlay->setDragState(false, false);
    }

    if (!standaloneMode || standalonePanel == nullptr)
    {
        return;
    }

    for (const auto& filePath : files)
    {
        if (AudioFileUtils::isSupportedAudioFile(filePath))
        {
            if (const juce::File audioFile(filePath); audioFile.existsAsFile())
            {
                standalonePanel->loadFile(audioFile);
                return;
            }
        }
    }
}
