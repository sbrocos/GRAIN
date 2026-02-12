/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

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

    // processBlock sub-methods (refactor for readability)
    void updateParameterTargets();
    void processWetOversampled(juce::dsp::AudioBlock<float>& oversampledBlock);
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
    GrainDSP::FocusMode lastFocusMode = GrainDSP::FocusMode::Mid;

    // Internal oversampling (Task 007)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    int currentOversamplingOrder = 1;    // 2^1 = 2× real-time, 2^2 = 4× offline
    juce::AudioBuffer<float> dryBuffer;  // Pre-allocated dry signal copy

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRAINAudioProcessor)
};
