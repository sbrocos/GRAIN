/*
  ==============================================================================

    GrainLookAndFeel.cpp
    GRAIN — Custom LookAndFeel with dot-style rotary knobs.
    Implementation based on the approved JSX mockup.

  ==============================================================================
*/

#include "GrainLookAndFeel.h"

//==============================================================================
GrainLookAndFeel::GrainLookAndFeel()
{
    // Set default label color for light background
    setColour(juce::Label::textColourId, GrainColours::kText);

    // Slider text box colors (value readout below knob)
    setColour(juce::Slider::textBoxTextColourId, GrainColours::kText);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, GrainColours::kAccent.withAlpha(0.3f));

    // Label background transparent (for slider value labels)
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

    // ComboBox (Focus selector — until replaced by TextButtons in subtask 5)
    setColour(juce::ComboBox::backgroundColourId, GrainColours::kSurface);
    setColour(juce::ComboBox::textColourId, GrainColours::kTextBright);
    setColour(juce::ComboBox::arrowColourId, GrainColours::kTextMuted);
    setColour(juce::ComboBox::outlineColourId, GrainColours::kSurfaceLight);

    // PopupMenu (for ComboBox dropdown)
    setColour(juce::PopupMenu::backgroundColourId, GrainColours::kSurface);
    setColour(juce::PopupMenu::textColourId, GrainColours::kTextBright);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, GrainColours::kAccent);
    setColour(juce::PopupMenu::highlightedTextColourId, GrainColours::kTextBright);

    // TextButton (Bypass — until restyled in subtask 4)
    setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    setColour(juce::TextButton::buttonOnColourId, GrainColours::kAccent);
    setColour(juce::TextButton::textColourOffId, GrainColours::kTextBright);
    setColour(juce::TextButton::textColourOnId, GrainColours::kTextBright);
}

//==============================================================================
// NOLINTNEXTLINE(readability-function-size) — signature imposed by juce::LookAndFeel
void GrainLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    constexpr float cornerSize = 4.0f;

    auto baseColour = backgroundColour;

    if (shouldDrawButtonAsDown)
    {
        baseColour = baseColour.darker(0.2f);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColour = baseColour.brighter(0.1f);
    }

    // Fill
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border — same colour family as button background
    g.setColour(GrainColours::kTransportButton);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

//==============================================================================
void GrainLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    // Only draw text, skip background/outline entirely
    if (label.isBeingEdited())
    {
        return;
    }

    auto textColour = label.findColour(juce::Label::textColourId);
    g.setColour(textColour);
    g.setFont(label.getFont());
    g.drawText(label.getText(), label.getLocalBounds().toFloat(), label.getJustificationType(), false);
}

//==============================================================================
juce::Font GrainLookAndFeel::getSliderFont()
{
    return {juce::FontOptions("Roboto", 14.0f, juce::Font::plain)};
}

//==============================================================================
namespace
{

/** Size-dependent drawing parameters for dot-style knobs. */
struct KnobSizeParams
{
    int dotCount;
    float dotRadius;
    float knobRadius;
    float indicatorRadius;
    float indicatorInset;  // distance from knob edge to indicator center

    static KnobSizeParams fromWidth(int width)
    {
        if (width >= 140)
        {
            return {35, 3.0f, 60.0f, 5.5f, 22.0f};  // Large: GRAIN, WARM
        }
        if (width >= 100)
        {
            return {24, 3.5f, 40.0f, 4.0f, 16.0f};  // Medium: MIX
        }
        return {21, 2.5f, 25.0f, 3.0f, 10.0f};  // Small: INPUT, OUTPUT
    }
};

/** Draw the arc of dots around a knob. */
// NOLINTNEXTLINE(readability-function-size) — many params needed for drawing context
void drawDotArc(juce::Graphics& g, juce::Point<float> centre, float arcRadius, float dotRadius, int dotCount,
                float sliderPos, float startAngle, float endAngle)
{
    for (int i = 0; i < dotCount; ++i)
    {
        const float proportion = static_cast<float>(i) / static_cast<float>(dotCount - 1);
        const float angle = startAngle + (proportion * (endAngle - startAngle));
        const float cosA = std::cos(angle - juce::MathConstants<float>::halfPi);
        const float sinA = std::sin(angle - juce::MathConstants<float>::halfPi);

        const float dotX = centre.x + (arcRadius * cosA);
        const float dotY = centre.y + (arcRadius * sinA);

        const bool isActive = proportion <= sliderPos;
        g.setColour(isActive ? GrainColours::kAccent : GrainColours::kSurface);
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }
}

/** Draw the knob body (shadow + gradient circle + inner ring). */
void drawKnobBody(juce::Graphics& g, juce::Point<float> centre, float kr)
{
    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(centre.x - kr - 1.0f, centre.y - kr + 1.0f, (kr * 2.0f) + 2.0f, (kr * 2.0f) + 2.0f);

    // Gradient body
    const juce::ColourGradient gradient(juce::Colour(0xff2a2a2a), centre.x - (kr * 0.5f), centre.y - (kr * 0.5f),
                                        juce::Colour(0xff1a1a1a), centre.x + (kr * 0.5f), centre.y + (kr * 0.5f),
                                        false);
    g.setGradientFill(gradient);
    g.fillEllipse(centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f);

    // Inner ring (2px border, inset 3px)
    constexpr float kRingInset = 3.0f;
    g.setColour(GrainColours::kSurfaceLight);
    g.drawEllipse(centre.x - kr + kRingInset, centre.y - kr + kRingInset, (kr - kRingInset) * 2.0f,
                  (kr - kRingInset) * 2.0f, 2.0f);
}

/** Draw the white indicator dot on the knob face. */
void drawIndicatorDot(juce::Graphics& g, juce::Point<float> centre, float kr, float indicatorInset,
                      float indicatorRadius, float angle)
{
    const float distance = kr - indicatorInset;
    const float cosA = std::cos(angle - juce::MathConstants<float>::halfPi);
    const float sinA = std::sin(angle - juce::MathConstants<float>::halfPi);

    const float indX = centre.x + (distance * cosA);
    const float indY = centre.y + (distance * sinA);

    // Glow
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillEllipse(indX - indicatorRadius - 1.0f, indY - indicatorRadius - 1.0f, (indicatorRadius + 1.0f) * 2.0f,
                  (indicatorRadius + 1.0f) * 2.0f);

    // Dot
    g.setColour(juce::Colours::white);
    g.fillEllipse(indX - indicatorRadius, indY - indicatorRadius, indicatorRadius * 2.0f, indicatorRadius * 2.0f);
}

}  // namespace

//==============================================================================
// NOLINTNEXTLINE(readability-function-size) — signature imposed by juce::LookAndFeel
void GrainLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& /*slider*/)
{
    const auto params = KnobSizeParams::fromWidth(width);
    const auto centre = juce::Rectangle<int>(x, y, width, height).toFloat().getCentre();

    // 1. Dot arc
    drawDotArc(g, centre, params.knobRadius + 15.0f, params.dotRadius, params.dotCount, sliderPos, rotaryStartAngle,
               rotaryEndAngle);

    // 2. Knob body (shadow + gradient + inner ring)
    drawKnobBody(g, centre, params.knobRadius);

    // 3. Indicator dot
    const float indicatorAngle = rotaryStartAngle + (sliderPos * (rotaryEndAngle - rotaryStartAngle));
    drawIndicatorDot(g, centre, params.knobRadius, params.indicatorInset, params.indicatorRadius, indicatorAngle);
}
