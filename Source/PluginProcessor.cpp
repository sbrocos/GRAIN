/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

// #include "PluginEditor.h"  // Not needed while using GenericAudioProcessorEditor

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GRAINAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("drive", 1), "Drive", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1), "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("output", 1), "Output", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("bypass", 1), "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("warmth", 1), "Warmth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("focus", 1), "Focus",
                                                                  juce::StringArray{"Low", "Mid", "High"}, 1));

    return {params.begin(), params.end()};
}

//==============================================================================
GRAINAudioProcessor::GRAINAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
    #if !JucePlugin_IsMidiEffect
        #if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
        #endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
                         )
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
    , driveParam(apvts.getRawParameterValue("drive"))
    , mixParam(apvts.getRawParameterValue("mix"))
    , outputParam(apvts.getRawParameterValue("output"))
    , warmthParam(apvts.getRawParameterValue("warmth"))
    , bypassParam(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("bypass")))
    , focusParam(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("focus")))
#endif
{
}

GRAINAudioProcessor::~GRAINAudioProcessor() = default;

//==============================================================================
const juce::String GRAINAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GRAINAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool GRAINAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool GRAINAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double GRAINAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GRAINAudioProcessor::getNumPrograms()
{
    return 1;  // NB: some hosts don't cope very well if you tell them there are 0 programs,
               // so this should be at least 1, even if you're not really implementing programs.
}

int GRAINAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GRAINAudioProcessor::setCurrentProgram(int index) {}

const juce::String GRAINAudioProcessor::getProgramName(int /*index*/)
{
    return {};
}

void GRAINAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

//==============================================================================
void GRAINAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // --- Oversampling setup (Task 007) ---
    // 2^1 = 2× for real-time, 2^2 = 4× for offline bounce
    currentOversamplingOrder = isNonRealtime() ? 2 : 1;

    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(getTotalNumInputChannels()), currentOversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

    oversampling->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Report latency to host for compensation
    setLatencySamples(static_cast<int>(oversampling->getLatencyInSamples()));

    // Calculate oversampled rate for DSP modules that run in the wet path
    const double oversampledRate = sampleRate * static_cast<double>(oversampling->getOversamplingFactor());

    // --- Smoothers ---
    // Drive/warmth run at OVERSAMPLED rate (inside wet processing loop)
    driveSmoothed.reset(oversampledRate, 0.02);
    warmthSmoothed.reset(oversampledRate, 0.02);

    // Mix/gain run at ORIGINAL rate (after downsampling, linear operations)
    mixSmoothed.reset(sampleRate, 0.02);
    gainSmoothed.reset(sampleRate, 0.02);

    // Set initial values
    driveSmoothed.setCurrentAndTargetValue(*driveParam);
    warmthSmoothed.setCurrentAndTargetValue(*warmthParam);

    const bool bypass = bypassParam->get();
    const float targetMix = bypass ? 0.0f : static_cast<float>(*mixParam);
    mixSmoothed.setCurrentAndTargetValue(targetMix);

    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(*outputParam)));

    // --- RMS detector at oversampled rate (Task 003) ---
    rmsDetector.prepare(static_cast<float>(oversampledRate), calibration.rms);
    rmsDetector.reset();
    currentEnvelope = 0.0f;

    // --- Per-channel pipelines at oversampled rate (Task 006b/006c/007b) ---
    const auto focusMode = static_cast<GrainDSP::FocusMode>(focusParam->getIndex());
    lastFocusMode = focusMode;
    pipelineLeft.prepare(static_cast<float>(oversampledRate), focusMode, calibration);
    pipelineLeft.reset();
    pipelineRight.prepare(static_cast<float>(oversampledRate), focusMode, calibration);
    pipelineRight.reset();

    // --- Pre-allocate dry buffer (avoid real-time allocation) ---
    dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
}

void GRAINAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GRAINAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
    #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    {
        return false;
    }

        // This checks if the input layout matches the output layout
        #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    {
        return false;
    }
        #endif

    return true;
    #endif
}
#endif

void GRAINAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    const juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that don't have input data
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    // Save dry signal at original rate (before upsampling)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }

    updateParameterTargets();

    // Upsample → wet DSP → downsample
    juce::dsp::AudioBlock<float> block(buffer);
    auto oversampledBlock = oversampling->processSamplesUp(block);
    processWetOversampled(oversampledBlock);
    oversampling->processSamplesDown(block);

    // Linear stages at original rate
    applyMixAndGain(buffer);
}

//==============================================================================
void GRAINAudioProcessor::updateParameterTargets()
{
    // Bypass drives mix to 0 (soft bypass via smoothing)
    const bool bypass = bypassParam->get();
    const float targetMix = bypass ? 0.0f : static_cast<float>(*mixParam);

    // Check if focus mode changed — use oversampled rate for coefficients
    const auto currentFocus = static_cast<GrainDSP::FocusMode>(focusParam->getIndex());
    if (currentFocus != lastFocusMode)
    {
        const auto oversampledRate =
            static_cast<float>(getSampleRate() * static_cast<double>(oversampling->getOversamplingFactor()));
        pipelineLeft.setFocusMode(oversampledRate, currentFocus);
        pipelineRight.setFocusMode(oversampledRate, currentFocus);
        lastFocusMode = currentFocus;
    }

    // Drive/warmth targets (smoothed at oversampled rate)
    driveSmoothed.setTargetValue(*driveParam);
    warmthSmoothed.setTargetValue(*warmthParam);

    // Mix/gain targets (smoothed at original rate)
    mixSmoothed.setTargetValue(targetMix);
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(*outputParam)));
}

//==============================================================================
void GRAINAudioProcessor::processWetOversampled(juce::dsp::AudioBlock<float>& oversampledBlock)
{
    const auto numSamples = static_cast<int>(oversampledBlock.getNumSamples());
    const auto numChannels = static_cast<int>(oversampledBlock.getNumChannels());

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Per-sample smoothing at oversampled rate
        const float drive = driveSmoothed.getNextValue();
        const float warmth = warmthSmoothed.getNextValue();

        // Linked stereo RMS: mono sum of both channels
        float monoInput = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            monoInput += oversampledBlock.getSample(ch, sample);
        }
        monoInput /= static_cast<float>(numChannels);
        currentEnvelope = rmsDetector.process(monoInput);

        // Process left channel wet path
        if (numChannels > 0)
        {
            const float input = oversampledBlock.getSample(0, sample);
            const float wet = pipelineLeft.processWet(input, currentEnvelope, drive, warmth);
            oversampledBlock.setSample(0, sample, wet);
        }

        // Process right channel wet path
        if (numChannels > 1)
        {
            const float input = oversampledBlock.getSample(1, sample);
            const float wet = pipelineRight.processWet(input, currentEnvelope, drive, warmth);
            oversampledBlock.setSample(1, sample, wet);
        }
    }
}

//==============================================================================
void GRAINAudioProcessor::applyMixAndGain(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = getTotalNumInputChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mix = mixSmoothed.getNextValue();
        const float gain = gainSmoothed.getNextValue();

        if (numChannels > 0)
        {
            const float dry = dryBuffer.getSample(0, sample);
            const float wet = buffer.getSample(0, sample);
            buffer.setSample(0, sample, pipelineLeft.processMixGain(dry, wet, mix, gain));
        }

        if (numChannels > 1)
        {
            const float dry = dryBuffer.getSample(1, sample);
            const float wet = buffer.getSample(1, sample);
            buffer.setSample(1, sample, pipelineRight.processMixGain(dry, wet, mix, gain));
        }
    }
}

//==============================================================================
bool GRAINAudioProcessor::hasEditor() const
{
    return true;  // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GRAINAudioProcessor::createEditor()
{
    // Temporary generic UI for DSP development and listening tests
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void GRAINAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter state
    auto state = apvts.copyState();
    const std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GRAINAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GRAINAudioProcessor();
}
