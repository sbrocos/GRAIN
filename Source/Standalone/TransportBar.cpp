/*
  ==============================================================================

    TransportBar.cpp
    GRAIN — Standalone transport bar UI component implementation (GT-17).

  ==============================================================================
*/

#include "TransportBar.h"

#include "../GrainColours.h"

namespace
{
constexpr int kButtonWidth = 60;
constexpr int kButtonGap = 6;
constexpr int kProgressBarHeight = 8;
constexpr int kControlRowHeight = 30;
}  // namespace

//==============================================================================
TransportBar::TransportBar(FilePlayerSource& filePlayer) : player(filePlayer)
{
    // Open button
    openButton.setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    openButton.setColour(juce::TextButton::textColourOffId, GrainColours::kText);
    openButton.onClick = [this]() { listeners.call(&Listener::openFileRequested); };
    addAndMakeVisible(openButton);

    // Stop button — rewinds to start
    stopButton.setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    stopButton.setColour(juce::TextButton::textColourOffId, GrainColours::kText);
    stopButton.onClick = [this]()
    {
        player.stop();
        player.seekToPosition(0.0);
        listeners.call(&Listener::stopRequested);
    };
    addAndMakeVisible(stopButton);

    // Play/Pause button
    playPauseButton.setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    playPauseButton.setColour(juce::TextButton::textColourOffId, GrainColours::kText);
    playPauseButton.onClick = [this]()
    {
        if (player.isPlaying())
        {
            player.stop();
        }
        else
        {
            player.play();
        }
    };
    addAndMakeVisible(playPauseButton);

    // Loop button (toggle)
    loopButton.setButtonText("Loop");
    loopButton.setClickingTogglesState(true);
    loopButton.setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    loopButton.setColour(juce::TextButton::buttonOnColourId, GrainColours::kAccent);
    loopButton.setColour(juce::TextButton::textColourOffId, GrainColours::kText);
    loopButton.setColour(juce::TextButton::textColourOnId, GrainColours::kTextBright);
    loopButton.onClick = [this]() { player.setLooping(loopButton.getToggleState()); };
    addAndMakeVisible(loopButton);

    // Export button
    exportButton.setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    exportButton.setColour(juce::TextButton::textColourOffId, GrainColours::kText);
    exportButton.onClick = [this]() { listeners.call(&Listener::exportRequested); };
    addAndMakeVisible(exportButton);

    // Register as transport listener
    player.addListener(this);

    // Initial state
    updateButtonStates();

    // Start timer for position/progress updates (30 FPS)
    startTimerHz(30);
}

TransportBar::~TransportBar()
{
    stopTimer();
    player.removeListener(this);
}

//==============================================================================
void TransportBar::paint(juce::Graphics& g)
{
    g.fillAll(GrainColours::kSurface);

    // Time text
    auto bounds = getLocalBounds();
    auto controlRow = bounds.removeFromTop(kControlRowHeight);

    // Time display on the right side of the control row
    g.setColour(GrainColours::kText);
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText(timeText, controlRow.removeFromRight(140).reduced(4, 0), juce::Justification::centredRight);

    // Progress bar
    auto progressBounds = getProgressBarBounds();

    // Track background
    g.setColour(GrainColours::kBackground);
    g.fillRoundedRectangle(progressBounds.toFloat(), 3.0f);

    // Filled progress
    if (progressNormalized > 0.0f)
    {
        auto filledWidth = static_cast<int>(static_cast<float>(progressBounds.getWidth()) * progressNormalized);
        auto filledBounds = progressBounds.withWidth(filledWidth);
        g.setColour(GrainColours::kAccent);
        g.fillRoundedRectangle(filledBounds.toFloat(), 3.0f);
    }

    // Playhead indicator
    if (player.isFileLoaded() && progressNormalized > 0.0f)
    {
        auto playheadX = progressBounds.getX() +
                         static_cast<int>(static_cast<float>(progressBounds.getWidth()) * progressNormalized);
        g.setColour(GrainColours::kTextBright);
        g.fillEllipse(static_cast<float>(playheadX) - 5.0f, static_cast<float>(progressBounds.getCentreY()) - 5.0f,
                      10.0f, 10.0f);
    }
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds();
    auto controlRow = bounds.removeFromTop(kControlRowHeight);
    controlRow.reduce(8, 2);

    openButton.setBounds(controlRow.removeFromLeft(kButtonWidth).reduced(0, 0));
    controlRow.removeFromLeft(kButtonGap);

    stopButton.setBounds(controlRow.removeFromLeft(kButtonWidth).reduced(0, 0));
    controlRow.removeFromLeft(kButtonGap);

    playPauseButton.setBounds(controlRow.removeFromLeft(kButtonWidth).reduced(0, 0));
    controlRow.removeFromLeft(kButtonGap);

    loopButton.setBounds(controlRow.removeFromLeft(kButtonWidth).reduced(0, 0));
    controlRow.removeFromLeft(kButtonGap);

    exportButton.setBounds(controlRow.removeFromLeft(kButtonWidth).reduced(0, 0));
}

void TransportBar::mouseDown(const juce::MouseEvent& event)
{
    auto progressBounds = getProgressBarBounds();

    if (progressBounds.contains(event.getPosition()) && player.isFileLoaded())
    {
        auto relativeX = static_cast<float>(event.getPosition().getX() - progressBounds.getX());
        auto normalized = relativeX / static_cast<float>(progressBounds.getWidth());
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        auto seekPosition = static_cast<double>(normalized) * player.getFileDurationSeconds();
        player.seekToPosition(seekPosition);
    }
}

//==============================================================================
void TransportBar::transportStateChanged(bool /*isNowPlaying*/)
{
    // Must be called on the message thread (JUCE ChangeListener guarantees this)
    updateButtonStates();
}

void TransportBar::transportReachedEnd()
{
    // No specific action needed here for the UI
}

//==============================================================================
void TransportBar::addListener(Listener* listener)
{
    listeners.add(listener);
}

void TransportBar::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

//==============================================================================
void TransportBar::updateButtonStates()
{
    const bool fileLoaded = player.isFileLoaded();
    const bool playing = player.isPlaying();

    playPauseButton.setButtonText(playing ? "Pause" : "Play");
    playPauseButton.setEnabled(fileLoaded);
    stopButton.setEnabled(fileLoaded);
    loopButton.setEnabled(fileLoaded);
    exportButton.setEnabled(fileLoaded && !playing);
}

//==============================================================================
void TransportBar::timerCallback()
{
    if (player.isFileLoaded())
    {
        const double currentPos = player.getCurrentPosition();
        const double totalDuration = player.getFileDurationSeconds();

        timeText = formatTime(currentPos) + " / " + formatTime(totalDuration);

        if (totalDuration > 0.0)
        {
            progressNormalized = static_cast<float>(currentPos / totalDuration);
            progressNormalized = juce::jlimit(0.0f, 1.0f, progressNormalized);
        }
        else
        {
            progressNormalized = 0.0f;
        }
    }
    else
    {
        timeText = "-- / --";
        progressNormalized = 0.0f;
    }

    repaint();
}

//==============================================================================
juce::String TransportBar::formatTime(double seconds)
{
    seconds = std::max(seconds, 0.0);

    const int totalSeconds = static_cast<int>(seconds);
    const int minutes = totalSeconds / 60;
    const int secs = totalSeconds % 60;

    return juce::String::formatted("%02d:%02d", minutes, secs);
}

juce::Rectangle<int> TransportBar::getProgressBarBounds() const
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(kControlRowHeight);
    return bounds.reduced(8, 4).withHeight(kProgressBarHeight);
}
