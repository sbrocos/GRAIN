/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include "DSP/GrainDSP.h"
// #include "PluginEditor.h"  // Not needed while using GenericAudioProcessorEditor
#include "Tests/DSPTests.cpp"  // NOLINT(bugprone-suspicious-include)

//==============================================================================
// Debug-only test runner: runs unit tests when plugin loads
#if JUCE_DEBUG
namespace
{
struct TestRunner
{
    TestRunner()
    {
        juce::UnitTestRunner runner;
        runner.runAllTests();
    }
} testRunner;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp)
}  // namespace
#endif

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

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("bypass", 1), "Bypass", juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("warmth", 1), "Warmth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

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
    , bypassParam(apvts.getRawParameterValue("bypass"))
    , warmthParam(apvts.getRawParameterValue("warmth"))
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
void GRAINAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    // Initialize smoothed values with 20ms smoothing time
    driveSmoothed.reset(sampleRate, 0.02);
    mixSmoothed.reset(sampleRate, 0.02);
    gainSmoothed.reset(sampleRate, 0.02);

    warmthSmoothed.reset(sampleRate, 0.02);

    // Set initial values
    driveSmoothed.setCurrentAndTargetValue(*driveParam);
    warmthSmoothed.setCurrentAndTargetValue(*warmthParam);

    const bool bypass = *bypassParam > 0.5f;
    const float targetMix = bypass ? 0.0f : static_cast<float>(*mixParam);
    mixSmoothed.setCurrentAndTargetValue(targetMix);

    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(*outputParam)));

    // Initialize RMS detector (Task 003)
    rmsDetector.prepare(static_cast<float>(sampleRate), GrainDSP::kRmsAttackMs, GrainDSP::kRmsReleaseMs);
    rmsDetector.reset();
    currentEnvelope = 0.0f;

    // Initialize DC blockers (Task 004)
    dcBlockerLeft.prepare(static_cast<float>(sampleRate));
    dcBlockerLeft.reset();
    dcBlockerRight.prepare(static_cast<float>(sampleRate));
    dcBlockerRight.reset();
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
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    // Bypass via mix: when bypassed, mix target = 0 (full dry)
    const bool bypass = *bypassParam > 0.5f;
    const float targetMix = bypass ? 0.0f : static_cast<float>(*mixParam);

    // Update targets at block start
    driveSmoothed.setTargetValue(*driveParam);
    warmthSmoothed.setTargetValue(*warmthParam);
    mixSmoothed.setTargetValue(targetMix);
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(*outputParam)));

    // Get channel pointers
    auto numSamples = buffer.getNumSamples();
    float* leftChannel = totalNumInputChannels > 0 ? buffer.getWritePointer(0) : nullptr;
    float* rightChannel = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Process sample-by-sample (RMS detector needs to run once per frame)
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Per-sample smoothing (NOT per-block!)
        const float drive = driveSmoothed.getNextValue();
        const float warmth = warmthSmoothed.getNextValue();
        const float mix = mixSmoothed.getNextValue();
        const float gain = gainSmoothed.getNextValue();

        // Get samples from both channels
        float const leftSample = leftChannel != nullptr ? leftChannel[sample] : 0.0f;
        float const rightSample = rightChannel != nullptr ? rightChannel[sample] : leftSample;

        // RMS detector: process once per sample-frame with mono sum (linked stereo)
        const float monoInput = (leftSample + rightSample) * 0.5f;
        currentEnvelope = rmsDetector.process(monoInput);

        // Process each channel through the full DSP chain
        if (leftChannel != nullptr)
        {
            leftChannel[sample] =
                GrainDSP::processSample(leftSample, currentEnvelope, drive, warmth, mix, gain, dcBlockerLeft);
        }

        if (rightChannel != nullptr)
        {
            rightChannel[sample] =
                GrainDSP::processSample(rightSample, currentEnvelope, drive, warmth, mix, gain, dcBlockerRight);
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
