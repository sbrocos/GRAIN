/*
  ==============================================================================

    StandalonePanel.cpp
    GRAIN — Standalone-only panel: waveform display + transport bar,
    owning the file player, recorder, and managing the export workflow.

  ==============================================================================
*/

#include "StandalonePanel.h"

//==============================================================================
StandalonePanel::StandalonePanel(GRAINAudioProcessor& p) : processor(p)
{
    filePlayer = std::make_unique<FilePlayerSource>();
    filePlayer->addListener(this);

    waveformDisplay = std::make_unique<WaveformDisplay>(*filePlayer);
    addAndMakeVisible(waveformDisplay.get());

    transportBar = std::make_unique<TransportBar>(*filePlayer);
    transportBar->addListener(this);
    addAndMakeVisible(transportBar.get());

    recorder = std::make_unique<AudioRecorder>();

    // Register file player, waveform display, and recorder with the processor
    processor.setFilePlayerSource(filePlayer.get());
    processor.setWaveformDisplay(waveformDisplay.get());
    processor.setAudioRecorder(recorder.get());
}

StandalonePanel::~StandalonePanel()
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

//==============================================================================
void StandalonePanel::resized()
{
    auto bounds = getLocalBounds();
    transportBar->setBounds(bounds.removeFromBottom(50));
    waveformDisplay->setBounds(bounds);
}

//==============================================================================
bool StandalonePanel::isFileLoaded() const
{
    return filePlayer != nullptr && filePlayer->isFileLoaded();
}

void StandalonePanel::loadFile(const juce::File& file)
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
// TransportBar::Listener

void StandalonePanel::openFileRequested()
{
    if (filePlayer == nullptr)
    {
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser>("Open Audio File", juce::File(), "*.wav;*.aiff;*.aif");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags,
                             [this](const juce::FileChooser& fc)
                             {
                                 const auto result = fc.getResult();

                                 if (result.existsAsFile())
                                 {
                                     loadFile(result);
                                 }
                             });
}

void StandalonePanel::stopRequested()
{
    if (waveformDisplay != nullptr)
    {
        waveformDisplay->clearWetBuffer();
    }
}

void StandalonePanel::exportRequested()
{
    if (filePlayer == nullptr || !filePlayer->isFileLoaded() || recorder == nullptr)
    {
        return;
    }

    const auto loadedFile = filePlayer->getLoadedFile();
    const auto defaultName = loadedFile.getFileNameWithoutExtension() + "_processed.wav";
    const auto defaultDir = loadedFile.getParentDirectory();

    fileChooser =
        std::make_unique<juce::FileChooser>("Export Processed Audio", defaultDir.getChildFile(defaultName), "*.wav");

    const auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                              juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(chooserFlags,
                             [this](const juce::FileChooser& fc)
                             {
                                 const auto result = fc.getResult();

                                 if (result == juce::File())
                                 {
                                     return;  // User cancelled
                                 }

                                 auto outputFile = result;
                                 if (outputFile.getFileExtension().toLowerCase() != ".wav")
                                 {
                                     outputFile = outputFile.withFileExtension(".wav");
                                 }

                                 const auto sampleRate = filePlayer->getFileSampleRate();
                                 const auto numChannels = filePlayer->getFileNumChannels();

                                 if (!recorder->startRecording(outputFile, sampleRate, numChannels))
                                 {
                                     return;
                                 }

                                 exporting = true;

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
// FilePlayerSource::Listener

void StandalonePanel::transportStateChanged(bool /*isNowPlaying*/)
{
    if (transportBar != nullptr)
    {
        transportBar->updateButtonStates();
    }
}

void StandalonePanel::transportReachedEnd()
{
    if (exporting && recorder != nullptr)
    {
        recorder->stopRecording();
        exporting = false;

        if (transportBar != nullptr)
        {
            transportBar->updateButtonStates();
        }
    }
}
