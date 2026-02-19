/*
  ==============================================================================

    MeterPanel.cpp
    GRAIN — Single-channel segmented LED meter with peak hold.

  ==============================================================================
*/

#include "MeterPanel.h"

#include "../../GrainColours.h"

//==============================================================================
MeterPanel::MeterPanel(std::atomic<float>& l, std::atomic<float>& r, const juce::String& label)
    : atomicL(l), atomicR(r), meterLabel(label)
{
}

//==============================================================================
void MeterPanel::updateLevels()
{
    const float inL = atomicL.load();
    const float inR = atomicR.load();

    displayL = std::max(inL, displayL * kMeterDecay);
    displayR = std::max(inR, displayR * kMeterDecay);

    peakHoldL.update(displayL);
    peakHoldR.update(displayR);

    repaint();
}

//==============================================================================
void MeterPanel::paint(juce::Graphics& g)
{
    drawMeter(g, getLocalBounds());
}

//==============================================================================
// NOLINTNEXTLINE(readability-function-size) — segmented meter rendering with glow + peak hold
void MeterPanel::drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    if (bounds.isEmpty())
    {
        return;
    }

    constexpr int kNumSegments = 32;
    constexpr int kSegmentWidth = 8;
    constexpr int kSegmentHeight = 7;
    constexpr int kSegmentGap = 2;
    constexpr int kChannelGap = 4;
    constexpr int kInternalPadding = 8;
    constexpr float kCornerRadius = 4.0f;

    // Label above meter
    auto labelArea = bounds.removeFromTop(20);
    g.setColour(GrainColours::kText);
    g.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));
    g.drawText(meterLabel, labelArea, juce::Justification::centred);

    // Dark background container
    g.setColour(GrainColours::kSurface);
    g.fillRoundedRectangle(bounds.toFloat(), kCornerRadius);

    // Convert gain to normalized 0..1 (dB-mapped, -60dB floor)
    auto gainToNormalized = [](float gain) -> float
    {
        const float db = juce::Decibels::gainToDecibels(gain, -60.0f);
        const float norm = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
        return juce::jlimit(0.0f, 1.0f, norm);
    };

    const float normL = gainToNormalized(displayL);
    const float normR = gainToNormalized(displayR);
    const float normPeakL = gainToNormalized(peakHoldL.peakLevel);
    const float normPeakR = gainToNormalized(peakHoldR.peakLevel);

    // Segment layout (two channel strips, centered)
    const int totalWidth = (kSegmentWidth * 2) + kChannelGap;
    const int startX = bounds.getCentreX() - (totalWidth / 2);
    const int bottomY = bounds.getBottom() - kInternalPadding;

    auto getSegmentColour = [](float position) -> juce::Colour
    {
        if (position > 0.9f)
        {
            return GrainColours::kMeterRed;
        }
        if (position > 0.75f)
        {
            return GrainColours::kMeterYellow;
        }
        return GrainColours::kMeterGreen;
    };

    auto drawChannel = [&](int x, float level, float peak)
    {
        const int litSegments = static_cast<int>(level * static_cast<float>(kNumSegments));
        const int peakSegment = juce::jmax(0, static_cast<int>(peak * static_cast<float>(kNumSegments)) - 1);

        for (int i = 0; i < kNumSegments; ++i)
        {
            const int segY = bottomY - ((i + 1) * (kSegmentHeight + kSegmentGap));
            const float position = static_cast<float>(i) / static_cast<float>(kNumSegments);
            const bool isLit = i < litSegments;
            const bool isPeak = (i == peakSegment) && (peak > level) && (peak > 0.01f);

            const auto segRect =
                juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(segY),
                                       static_cast<float>(kSegmentWidth), static_cast<float>(kSegmentHeight));

            if (isLit || isPeak)
            {
                auto colour = getSegmentColour(position);
                if (isPeak && !isLit)
                {
                    colour = colour.withAlpha(0.6f);
                }

                if (isLit)
                {
                    g.setColour(colour.withAlpha(0.15f));
                    g.fillRoundedRectangle(segRect.expanded(2.0f), 2.0f);
                }

                g.setColour(colour);
            }
            else
            {
                g.setColour(GrainColours::kMeterOff);
            }

            g.fillRoundedRectangle(segRect, 1.0f);
        }
    };

    drawChannel(startX, normL, normPeakL);
    drawChannel(startX + kSegmentWidth + kChannelGap, normR, normPeakR);
}
