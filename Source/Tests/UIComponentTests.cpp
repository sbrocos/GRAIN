/*
  ==============================================================================

    UIComponentTests.cpp
    Unit tests for the UI sub-components extracted from PluginEditor.

    Tests cover stateful logic only (peak hold, focus selector, drag overlay).
    Pure rendering components (HeaderPanel, FooterPanel) have no testable state.

  ==============================================================================
*/

#include "../UI/Components/DragDropOverlay.h"
#include "../UI/Components/MeterPanel.h"

#include <JuceHeader.h>

//==============================================================================
class PeakHoldTest : public juce::UnitTest
{
public:
    PeakHoldTest() : juce::UnitTest("GRAIN UI — PeakHold") {}

    void runTest() override
    {
        runPeakHoldUpdateTest();
        runPeakHoldDecayTest();
        runPeakHoldResetTest();
    }

private:
    //==========================================================================
    void runPeakHoldUpdateTest()
    {
        beginTest("PeakHold: new peak is captured immediately");

        PeakHold ph;
        ph.update(0.5f);

        expectWithinAbsoluteError(ph.peakLevel, 0.5f, 1e-5f);
        expectEquals(ph.holdCounter, 30);
    }

    //==========================================================================
    void runPeakHoldDecayTest()
    {
        beginTest("PeakHold: level decays after hold period expires");

        PeakHold ph;
        ph.update(1.0f);

        // Exhaust hold counter
        for (int i = 0; i < 30; ++i)
        {
            ph.update(0.0f);
        }

        // holdCounter is now 0 — next update should decay
        const float levelBefore = ph.peakLevel;
        ph.update(0.0f);

        expect(ph.peakLevel < levelBefore, "Peak level should decay after hold period");
        expectWithinAbsoluteError(ph.peakLevel, levelBefore * 0.95f, 1e-5f);
    }

    //==========================================================================
    void runPeakHoldResetTest()
    {
        beginTest("PeakHold: reset clears peak and counter");

        PeakHold ph;
        ph.update(0.8f);
        ph.reset();

        expectWithinAbsoluteError(ph.peakLevel, 0.0f, 1e-5f);
        expectEquals(ph.holdCounter, 0);
    }
};

//==============================================================================
class DragDropOverlayTest : public juce::UnitTest
{
public:
    DragDropOverlayTest() : juce::UnitTest("GRAIN UI — DragDropOverlay") {}

    void runTest() override
    {
        runInitialStateTest();
        runAcceptedStateTest();
        runRejectedStateTest();
        runClearedStateTest();
    }

private:
    //==========================================================================
    void runInitialStateTest()
    {
        beginTest("DragDropOverlay: initial state is not hovering");

        DragDropOverlay overlay;

        expect(!overlay.isDragHovering(), "Should not be hovering initially");
        expect(!overlay.isDragAccepted(), "Should not be accepted initially");
    }

    //==========================================================================
    void runAcceptedStateTest()
    {
        beginTest("DragDropOverlay: accepted drag state");

        DragDropOverlay overlay;
        overlay.setDragState(true, true);

        expect(overlay.isDragHovering(), "Should be hovering");
        expect(overlay.isDragAccepted(), "Should be accepted");
    }

    //==========================================================================
    void runRejectedStateTest()
    {
        beginTest("DragDropOverlay: rejected drag state");

        DragDropOverlay overlay;
        overlay.setDragState(true, false);

        expect(overlay.isDragHovering(), "Should be hovering");
        expect(!overlay.isDragAccepted(), "Should not be accepted");
    }

    //==========================================================================
    void runClearedStateTest()
    {
        beginTest("DragDropOverlay: state clears on drag exit");

        DragDropOverlay overlay;
        overlay.setDragState(true, true);
        overlay.setDragState(false, false);

        expect(!overlay.isDragHovering(), "Should not be hovering after clear");
        expect(!overlay.isDragAccepted(), "Should not be accepted after clear");
    }
};

//==============================================================================
static PeakHoldTest peakHoldTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
static DragDropOverlayTest dragDropOverlayTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
