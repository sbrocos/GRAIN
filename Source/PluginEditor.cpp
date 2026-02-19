/*
  ==============================================================================

    PluginEditor.cpp
    GRAIN — Micro-harmonic saturation processor.
    Plugin editor implementation: HTML/CSS/JS UI via WebBrowserComponent (Phase B).

  ==============================================================================
*/

#include "PluginEditor.h"

#include "PluginProcessor.h"

#include <BinaryData.h>

namespace
{
constexpr int kEditorWidth = 480;
constexpr int kWebViewHeight = 480;
constexpr int kTransportBarHeight = 50;  // standalone transport bar
constexpr int kWaveformHeight = 120;     // standalone waveform display
}  // namespace

//==============================================================================
// MIME type helper for resource provider

static const char* getMimeForExtension(const juce::String& extension)
{
    if (extension == "html")
        return "text/html";
    if (extension == "js")
        return "text/javascript";
    if (extension == "css")
        return "text/css";
    return "application/octet-stream";
}

//==============================================================================
// SinglePageBrowser — prevent navigation away from embedded UI

bool GRAINAudioProcessorEditor::SinglePageBrowser::pageAboutToLoad(const juce::String& newURL)
{
    return newURL == getResourceProviderRoot();
}

//==============================================================================
GRAINAudioProcessorEditor::GRAINAudioProcessorEditor(GRAINAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
    , standaloneMode(p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    , webView(juce::WebBrowserComponent::Options{}
                  .withNativeIntegrationEnabled()
                  .withOptionsFrom(driveSliderRelay)
                  .withOptionsFrom(warmthSliderRelay)
                  .withOptionsFrom(inputSliderRelay)
                  .withOptionsFrom(mixSliderRelay)
                  .withOptionsFrom(outputSliderRelay)
                  .withOptionsFrom(bypassToggleRelay)
                  .withOptionsFrom(focusComboRelay)
                  .withResourceProvider([this](const auto& url) { return getResource(url); }))
    , driveAttachment(*p.getAPVTS().getParameter("drive"), driveSliderRelay)
    , warmthAttachment(*p.getAPVTS().getParameter("warmth"), warmthSliderRelay)
    , inputAttachment(*p.getAPVTS().getParameter("inputGain"), inputSliderRelay)
    , mixAttachment(*p.getAPVTS().getParameter("mix"), mixSliderRelay)
    , outputAttachment(*p.getAPVTS().getParameter("output"), outputSliderRelay)
    , bypassAttachment(*p.getAPVTS().getParameter("bypass"), bypassToggleRelay)
    , focusAttachment(*p.getAPVTS().getParameter("focus"), focusComboRelay)
{
    // Keep LookAndFeel for standalone native components
    if (standaloneMode)
    {
        setLookAndFeel(&grainLookAndFeel);
    }

    addAndMakeVisible(webView);
    webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

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
    const int editorHeight = standaloneMode ? (kWebViewHeight + kWaveformHeight + kTransportBarHeight) : kWebViewHeight;
    setSize(kEditorWidth, editorHeight);

    // Start meter timer (60 Hz ~ 16ms)
    startTimerHz(60);
}

GRAINAudioProcessorEditor::~GRAINAudioProcessorEditor()
{
    if (standaloneMode)
    {
        setLookAndFeel(nullptr);
    }

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
std::optional<juce::WebBrowserComponent::Resource> GRAINAudioProcessorEditor::getResource(const juce::String& url)
{
    const auto path =
        (url == "/" || url.isEmpty()) ? juce::String("index.html") : url.fromFirstOccurrenceOf("/", false, false);

    // Map resource filenames to BinaryData
    struct ResourceEntry
    {
        const char* data;
        int size;
    };

    // BinaryData names: Projucer converts hyphens → underscores, dots → underscores
    // e.g., "grain-ui.js" → BinaryData::grainui_js
    static const std::unordered_map<std::string, ResourceEntry> resources = {
        {"index.html", {BinaryData::index_html, BinaryData::index_htmlSize}},
        {"grain-ui.js", {BinaryData::grainui_js, BinaryData::grainui_jsSize}},
        {"grain-ui.css", {BinaryData::grainui_css, BinaryData::grainui_cssSize}}};

    auto it = resources.find(path.toStdString());

    if (it != resources.end())
    {
        auto ext = path.fromLastOccurrenceOf(".", false, false).toLowerCase();

        std::vector<std::byte> data(static_cast<size_t>(it->second.size));
        std::memcpy(data.data(), it->second.data, data.size());

        return juce::WebBrowserComponent::Resource{std::move(data), getMimeForExtension(ext)};
    }

    return std::nullopt;
}

//==============================================================================
void GRAINAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all main UI rendering.
    // Only paint drag & drop overlay for standalone mode.
    if (dragHovering)
    {
        constexpr float kBorderWidth = 3.0f;
        auto borderBounds = getLocalBounds().toFloat().reduced(kBorderWidth * 0.5f);
        g.setColour(dragAccepted ? GrainColours::kAccent : GrainColours::kMeterRed);
        g.drawRoundedRectangle(borderBounds, 4.0f, kBorderWidth);
    }
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

    // WebView fills everything above standalone controls
    webView.setBounds(bounds);
}

//==============================================================================
// Standalone transport bar callbacks (GT-17) — unchanged from Phase A

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
    // Read atomic meter levels from the audio thread
    float const inL = processor.inputLevelL.load();
    float const inR = processor.inputLevelR.load();
    float const outL = processor.outputLevelL.load();
    float const outR = processor.outputLevelR.load();

    // Apply meter decay smoothing
    displayInputL = std::max(inL, displayInputL * kMeterDecay);
    displayInputR = std::max(inR, displayInputR * kMeterDecay);
    displayOutputL = std::max(outL, displayOutputL * kMeterDecay);
    displayOutputR = std::max(outR, displayOutputR * kMeterDecay);

    // Send meter update to web UI
    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("inL", displayInputL);
    payload->setProperty("inR", displayInputR);
    payload->setProperty("outL", displayOutputL);
    payload->setProperty("outR", displayOutputR);
    webView.emitEventIfBrowserIsVisible("meterUpdate", juce::var(payload.get()));
}
