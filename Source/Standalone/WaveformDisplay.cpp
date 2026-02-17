/*
  ==============================================================================

    WaveformDisplay.cpp
    GRAIN — Standalone waveform display implementation (GT-18).

  ==============================================================================
*/

#include "WaveformDisplay.h"

#include "../GrainColours.h"

namespace
{
constexpr int kPadding = 4;
constexpr float kDryAlpha = 0.35f;
constexpr float kWetAlpha = 0.8f;
constexpr float kCursorWidth = 2.0f;
}  // namespace

//==============================================================================
WaveformDisplay::WaveformDisplay(FilePlayerSource& filePlayer) : player(filePlayer)
{
    wetFifoBuffer.resize(2048, WetSample{});

    // Register as transport listener
    player.addListener(this);

    // Start timer for FIFO drain + repaint (30 FPS)
    startTimerHz(30);
}

WaveformDisplay::~WaveformDisplay()
{
    stopTimer();
    player.removeListener(this);
}

//==============================================================================
void WaveformDisplay::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(GrainColours::kBackground);

    auto bounds = getWaveformBounds();

    if (bounds.isEmpty())
    {
        return;
    }

    // Border
    g.setColour(GrainColours::kSurface);
    g.drawRect(bounds);

    if (!player.isFileLoaded())
    {
        // Draw placeholder text
        g.setColour(GrainColours::kText.withAlpha(0.4f));
        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        g.drawText("No file loaded", bounds, juce::Justification::centred);
        return;
    }

    // Draw dry waveform (from AudioThumbnail)
    drawDryWaveform(g, bounds);

    // Draw wet waveform overlay (from accumulated samples)
    drawWetWaveform(g, bounds);

    // Draw playback cursor
    drawCursor(g, bounds);
}

void WaveformDisplay::resized()
{
    // Resize wet columns to match display width
    auto bounds = getWaveformBounds();
    int const numColumns = bounds.getWidth();

    if (numColumns > 0 && static_cast<int>(wetColumns.size()) != numColumns)
    {
        wetColumns.resize(static_cast<size_t>(numColumns));
        clearWetBuffer();
    }
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getWaveformBounds();

    if (bounds.contains(event.getPosition()) && player.isFileLoaded())
    {
        float const normalized = pixelToNormalized(event.getPosition().getX());
        double const seekPosition = static_cast<double>(normalized) * player.getFileDurationSeconds();
        player.seekToPosition(seekPosition);
    }
}

//==============================================================================
void WaveformDisplay::transportStateChanged(bool /*isNowPlaying*/)
{
    // Repaint will be triggered by timer
}

void WaveformDisplay::transportReachedEnd()
{
    // Repaint will be triggered by timer
}

//==============================================================================
void WaveformDisplay::pushWetSamples(const float* samples, int numSamples, juce::int64 samplePosition)
{
    // Write to FIFO (audio thread safe)
    const int available = wetFifo.getFreeSpace();
    const int toWrite = std::min(numSamples, available);

    if (toWrite <= 0)
    {
        return;
    }

    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    wetFifo.prepareToWrite(toWrite, start1, size1, start2, size2);

    // Write position-tagged samples
    for (int i = 0; i < size1; ++i)
    {
        wetFifoBuffer[static_cast<size_t>(start1 + i)] = {samples[i], samplePosition + i};
    }

    for (int i = 0; i < size2; ++i)
    {
        wetFifoBuffer[static_cast<size_t>(start2 + i)] = {samples[size1 + i], samplePosition + size1 + i};
    }

    wetFifo.finishedWrite(size1 + size2);
}

void WaveformDisplay::clearWetBuffer()
{
    for (auto& col : wetColumns)
    {
        col.minVal = 0.0f;
        col.maxVal = 0.0f;
        col.sampleCount = 0;
    }

    // Drain any pending FIFO data
    const int available = wetFifo.getNumReady();
    if (available > 0)
    {
        int start1 = 0;
        int size1 = 0;
        int start2 = 0;
        int size2 = 0;
        wetFifo.prepareToRead(available, start1, size1, start2, size2);
        wetFifo.finishedRead(size1 + size2);
    }
}

bool WaveformDisplay::hasWetData() const
{
    return std::any_of(wetColumns.begin(), wetColumns.end(), [](const WetColumn& c) { return c.sampleCount > 0; });
}

//==============================================================================
float WaveformDisplay::pixelToNormalized(int pixelX) const
{
    auto bounds = getWaveformBounds();

    if (bounds.getWidth() <= 0)
    {
        return 0.0f;
    }

    auto const relative = static_cast<float>(pixelX - bounds.getX());
    return juce::jlimit(0.0f, 1.0f, relative / static_cast<float>(bounds.getWidth()));
}

int WaveformDisplay::normalizedToPixel(float normalized) const
{
    auto bounds = getWaveformBounds();
    return bounds.getX() + static_cast<int>(normalized * static_cast<float>(bounds.getWidth()));
}

juce::Rectangle<int> WaveformDisplay::getWaveformBounds() const
{
    return getLocalBounds().reduced(kPadding);
}

//==============================================================================
void WaveformDisplay::timerCallback()
{
    if (!player.isFileLoaded())
    {
        repaint();
        return;
    }

    // Drain FIFO and accumulate into wet columns
    const int numReady = wetFifo.getNumReady();

    if (numReady > 0)
    {
        int start1 = 0;
        int size1 = 0;
        int start2 = 0;
        int size2 = 0;
        wetFifo.prepareToRead(numReady, start1, size1, start2, size2);

        auto processChunk = [this](int start, int size)
        {
            if (size <= 0 || wetColumns.empty())
            {
                return;
            }

            auto const totalFileSamples = player.getFileLengthInSamples();

            if (totalFileSamples <= 0)
            {
                return;
            }

            const int numColumns = static_cast<int>(wetColumns.size());

            for (int i = 0; i < size; ++i)
            {
                auto const& ws = wetFifoBuffer[static_cast<size_t>(start + i)];

                // Map this sample to a column based on its actual file position
                auto const columnIndex = static_cast<int>(
                    (static_cast<double>(ws.filePosition) / static_cast<double>(totalFileSamples)) * numColumns);
                const int clampedCol = juce::jlimit(0, numColumns - 1, columnIndex);

                auto& col = wetColumns[static_cast<size_t>(clampedCol)];
                col.minVal = std::min(col.minVal, ws.value);
                col.maxVal = std::max(col.maxVal, ws.value);
                col.sampleCount++;
            }
        };

        processChunk(start1, size1);
        processChunk(start2, size2);

        wetFifo.finishedRead(size1 + size2);
    }

    repaint();
}

//==============================================================================
void WaveformDisplay::drawDryWaveform(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto& thumbnail = player.getThumbnail();

    if (thumbnail.getTotalLength() <= 0.0)
    {
        return;
    }

    // Draw mono (channel 0 only) to match the wet waveform representation.
    // The wet signal is captured from channel 0, so dry must be consistent.
    g.setColour(GrainColours::kText.withAlpha(kDryAlpha));
    thumbnail.drawChannel(g, bounds, 0.0, thumbnail.getTotalLength(), 0, 1.0f);
}

void WaveformDisplay::drawWetWaveform(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (wetColumns.empty() || !hasWetData())
    {
        return;
    }

    const int numColumns = static_cast<int>(wetColumns.size());
    auto const centreY = static_cast<float>(bounds.getCentreY());
    auto const halfHeight = static_cast<float>(bounds.getHeight()) * 0.5f;

    g.setColour(GrainColours::kAccent.withAlpha(kWetAlpha));

    for (int i = 0; i < numColumns; ++i)
    {
        auto const& col = wetColumns[static_cast<size_t>(i)];

        if (col.sampleCount == 0)
        {
            continue;
        }

        float const topY = centreY - (col.maxVal * halfHeight);
        float const bottomY = centreY - (col.minVal * halfHeight);
        auto const x = static_cast<float>(bounds.getX() + i);
        float const lineHeight = std::max(1.0f, bottomY - topY);

        g.fillRect(x, topY, 1.0f, lineHeight);
    }
}

void WaveformDisplay::drawCursor(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (!player.isFileLoaded() || player.getFileDurationSeconds() <= 0.0)
    {
        return;
    }

    auto const normalized = static_cast<float>(player.getCurrentPosition() / player.getFileDurationSeconds());
    const int cursorX = normalizedToPixel(juce::jlimit(0.0f, 1.0f, normalized));

    g.setColour(GrainColours::kTextBright);
    g.fillRect(static_cast<float>(cursorX) - (kCursorWidth * 0.5f), static_cast<float>(bounds.getY()), kCursorWidth,
               static_cast<float>(bounds.getHeight()));
}
