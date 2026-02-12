/*
  ==============================================================================

    PluginProcessor.h
    GRAIN — Micro-harmonic saturation processor.
    Main audio processor: parameter management, oversampling, and DSP orchestration.

    Signal flow:
    Input → [Upsample] → Dynamic Bias → Waveshaper → Warmth → Focus
          → [Downsample] → Mix (dry/wet) → DC Blocker → Output Gain

  ==============================================================================
*/

#pragma once

#include "DSP/GrainDSPPipeline.h"
#include "DSP/RMSDetector.h"
#include "DSP/SpectralFocus.h"

#include <juce_dsp/juce_dsp.h>

#include <JuceHeader.h>

//==============================================================================
/**
 * Main audio processor for the GRAIN plugin.
 *
 * Manages stereo processing via two mono DSPPipeline instances (L/R),
 * internal oversampling (2x real-time, 4x offline), and smooth parameter
 * transitions via SmoothedValue. Bypass is implemented as a soft fade
 * (mix target → 0) to avoid clicks.
 */
class GRAINAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    GRAINAudioProcessor();
    ~GRAINAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter state
    juce::AudioProcessorValueTreeState apvts;

private:
    //==============================================================================
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Read current parameter values and update smoother targets.
     *  Handles bypass (mix → 0), focus mode changes, and all smoother targets. */
    void updateParameterTargets();

    /** Run the nonlinear DSP chain (Bias → Waveshaper → Warmth → Focus)
     *  sample-by-sample at oversampled rate.
     *  @param oversampledBlock Audio block at oversampled rate (modified in-place) */
    void processWetOversampled(juce::dsp::AudioBlock<float>& oversampledBlock);

    /** Apply dry/wet mix, DC blocking, and output gain at original sample rate.
     *  @param buffer Audio buffer at original rate (modified in-place) */
    void applyMixAndGain(juce::AudioBuffer<float>& buffer);

    // Parameter pointers for fast access
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* warmthParam = nullptr;
    juce::AudioParameterBool* bypassParam = nullptr;
    juce::AudioParameterChoice* focusParam = nullptr;

    // Smoothed values for click-free parameter changes
    juce::SmoothedValue<float> driveSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> gainSmoothed;
    juce::SmoothedValue<float> warmthSmoothed;

    // RMS detector for Dynamic Bias (Task 003) — mono-summed, shared across channels
    GrainDSP::RMSDetector rmsDetector;
    float currentEnvelope = 0.0f;

    // Centralized calibration config (Task 007b)
    GrainDSP::CalibrationConfig calibration = GrainDSP::kDefaultCalibration;

    // Per-channel DSP pipelines (Task 006b)
    GrainDSP::DSPPipeline pipelineLeft;
    GrainDSP::DSPPipeline pipelineRight;

    // Spectral Focus mode tracking (Task 006c)
    GrainDSP::FocusMode lastFocusMode = GrainDSP::FocusMode::kMid;

    // Internal oversampling (Task 007)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    int currentOversamplingOrder = 1;    // 2^1 = 2× real-time, 2^2 = 4× offline
    juce::AudioBuffer<float> dryBuffer;  // Pre-allocated dry signal copy

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRAINAudioProcessor)
};
