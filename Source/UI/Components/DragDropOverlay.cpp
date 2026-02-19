/*
  ==============================================================================

    DragDropOverlay.cpp
    GRAIN — Drag & drop visual feedback overlay (border + color coding).

  ==============================================================================
*/

#include "DragDropOverlay.h"

#include "../../GrainColours.h"

//==============================================================================
DragDropOverlay::DragDropOverlay()
{
    // Transparent to mouse — only visual feedback
    setInterceptsMouseClicks(false, false);
}

//==============================================================================
void DragDropOverlay::setDragState(bool hovering, bool accepted)
{
    dragHovering = hovering;
    dragAccepted = accepted;
    repaint();
}

//==============================================================================
void DragDropOverlay::paint(juce::Graphics& g)
{
    if (!dragHovering)
    {
        return;
    }

    constexpr float kBorderWidth = 3.0f;
    auto borderBounds = getLocalBounds().toFloat().reduced(kBorderWidth * 0.5f);
    g.setColour(dragAccepted ? GrainColours::kAccent : GrainColours::kMeterRed);
    g.drawRoundedRectangle(borderBounds, 4.0f, kBorderWidth);
}
