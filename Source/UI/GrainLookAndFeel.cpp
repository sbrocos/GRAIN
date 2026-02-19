/*
  ==============================================================================

    GrainLookAndFeel.cpp
    GRAIN — Custom LookAndFeel: dot-style knobs, LED bypass, focus buttons,
    editable value fields. Pixel-perfect match to GrainUI.jsx mockup.

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

    // TextEditor colors (for editable slider value fields)
    setColour(juce::TextEditor::textColourId, GrainColours::kText);
    setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::highlightColourId, GrainColours::kAccent.withAlpha(0.3f));
    setColour(juce::TextEditor::highlightedTextColourId, GrainColours::kText);
    setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::CaretComponent::caretColourId, GrainColours::kAccent);

    // TextButton defaults
    setColour(juce::TextButton::buttonColourId, GrainColours::kSurface);
    setColour(juce::TextButton::buttonOnColourId, GrainColours::kAccent);
    setColour(juce::TextButton::textColourOffId, GrainColours::kTextBright);
    setColour(juce::TextButton::textColourOnId, GrainColours::kTextBright);
}

//==============================================================================
void GrainLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    // Skip drawing when being edited (TextEditor takes over)
    if (label.isBeingEdited())
    {
        return;
    }

    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());
    g.drawText(label.getText(), label.getLocalBounds().toFloat(), label.getJustificationType(), false);
}

//==============================================================================
juce::Label* GrainLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox(slider);

    label->setJustificationType(juce::Justification::centred);

    // When the Label spawns a TextEditor, reset its internal padding so
    // the editable text doesn't shift relative to the static label text.
    label->onEditorShow = [label]()
    {
        if (auto* editor = label->getCurrentTextEditor())
        {
            editor->setBorder(juce::BorderSize<int>(0));
            editor->setIndents(0, 0);
            editor->setJustification(juce::Justification::centred);
        }
    };

    return label;
}

//==============================================================================
void GrainLookAndFeel::fillTextEditorBackground(juce::Graphics& /*g*/, int /*width*/, int /*height*/,
                                                juce::TextEditor& /*editor*/)
{
    // Transparent — editing looks like inline text, not a text field
}

void GrainLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    // Subtle orange outline only when focused (actively editing)
    if (editor.hasKeyboardFocus(true))
    {
        g.setColour(GrainColours::kAccent.withAlpha(0.5f));
        g.drawRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 2.0f, 1.0f);
    }
}

//==============================================================================
// NOLINTNEXTLINE(readability-function-size) — multiple button styles via componentID dispatch
void GrainLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();

    if (button.getComponentID() == "bypass")
    {
        const bool isBypassed = button.getToggleState();

        if (!isBypassed)
        {
            // Processing: orange outer glow
            g.setColour(GrainColours::kBypassOn.withAlpha(0.3f));
            g.fillEllipse(bounds.expanded(2.0f));
        }

        // Circle body
        g.setColour(isBypassed ? GrainColours::kBypassOff : GrainColours::kBypassOn);
        g.fillEllipse(bounds.reduced(1.0f));

        if (isBypassed)
        {
            // Inset shadow effect
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.drawEllipse(bounds.reduced(2.0f), 1.5f);
        }

        return;
    }

    if (button.getComponentID() == "focusButton")
    {
        const bool isActive = button.getToggleState();

        if (isActive)
        {
            g.setColour(GrainColours::kAccent);
            g.fillRoundedRectangle(bounds, 4.0f);
        }
        // Inactive: transparent (no fill)

        return;
    }

    // Default fallback
    LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, shouldDrawButtonAsHighlighted,
                                         shouldDrawButtonAsDown);
}

void GrainLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    if (button.getComponentID() == "bypass")
    {
        const bool isBypassed = button.getToggleState();
        auto bounds = button.getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        const float iconSize = bounds.getWidth() * 0.4f;

        // Power icon: vertical line + 3/4 arc
        juce::Path powerIcon;

        // Vertical line (top segment)
        powerIcon.startNewSubPath(centre.x, centre.y - iconSize);
        powerIcon.lineTo(centre.x, centre.y - (iconSize * 0.2f));

        // Arc (open at top)
        const float arcRadius = iconSize * 0.7f;
        powerIcon.addArc(centre.x - arcRadius, centre.y - arcRadius, arcRadius * 2.0f, arcRadius * 2.0f,
                         -juce::MathConstants<float>::pi * 0.75f, juce::MathConstants<float>::pi * 0.75f, true);

        g.setColour(isBypassed ? juce::Colour(0xff666666) : juce::Colours::white);
        g.strokePath(powerIcon,
                     juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        return;
    }

    if (button.getComponentID() == "focusButton")
    {
        const bool isActive = button.getToggleState();
        g.setColour(isActive ? GrainColours::kTextBright : GrainColours::kTextMuted);
        g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);

        return;
    }

    // Default fallback
    LookAndFeel_V4::drawButtonText(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
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
        return {21, 3.0f, 25.0f, 3.0f, 10.0f};  // Small: INPUT, OUTPUT (dotRadius 3.0 per JSX)
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
