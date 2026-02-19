/*
  ==============================================================================

    DragDropOverlay.h
    GRAIN — Drag & drop visual feedback overlay (border + color coding).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Transparent overlay drawn over the entire plugin when a file is dragged over it.
 * Shows an orange border for accepted files, red for rejected ones.
 *
 * Usage: call setDragState() in fileDragEnter/Exit/filesDropped callbacks,
 * then place this component on top (setBounds(getLocalBounds()), setInterceptsMouseClicks(false, false)).
 */
class DragDropOverlay : public juce::Component
{
public:
    DragDropOverlay();
    ~DragDropOverlay() override = default;

    /** Update drag state and trigger repaint. */
    void setDragState(bool hovering, bool accepted);

    [[nodiscard]] bool isDragHovering() const { return dragHovering; }
    [[nodiscard]] bool isDragAccepted() const { return dragAccepted; }

    void paint(juce::Graphics& g) override;

private:
    bool dragHovering = false;
    bool dragAccepted = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragDropOverlay)
};
