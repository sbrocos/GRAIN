/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/GrainDSP.h"
#include "Tests/DSPTests.cpp"

//==============================================================================
// Debug-only test runner: runs unit tests when plugin loads
#if JUCE_DEBUG
static struct TestRunner
{
    TestRunner()
    {
        juce::UnitTestRunner runner;
        runner.runAllTests();
    }
} testRunner;
#endif

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GRAINAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("drive", 1),
        "Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("output", 1),
        "Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("bypass", 1),
        "Bypass",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f),
        0.0f
    ));

    return { params.begin(), params.end() };
}

//==============================================================================
GRAINAudioProcessor::GRAINAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Get parameter pointers for fast access in processBlock
    driveParam = apvts.getRawParameterValue("drive");
    mixParam = apvts.getRawParameterValue("mix");
    outputParam = apvts.getRawParameterValue("output");
    bypassParam = apvts.getRawParameterValue("bypass");
}

GRAINAudioProcessor::~GRAINAudioProcessor()
{
}

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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GRAINAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GRAINAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GRAINAudioProcessor::getProgramName (int index)
{
    return {};
}

void GRAINAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GRAINAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize smoothed values with 20ms smoothing time
    driveSmoothed.reset(sampleRate, 0.02);
    mixSmoothed.reset(sampleRate, 0.02);
    gainSmoothed.reset(sampleRate, 0.02);

    // Set initial values
    driveSmoothed.setCurrentAndTargetValue(*driveParam);

    bool bypass = *bypassParam > 0.5f;
    float targetMix = bypass ? 0.0f : static_cast<float>(*mixParam);
    mixSmoothed.setCurrentAndTargetValue(targetMix);

    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(*outputParam)));
}

void GRAINAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GRAINAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void GRAINAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Bypass via mix: when bypassed, mix target = 0 (full dry)
    bool bypass = *bypassParam > 0.5f;
    float targetMix = bypass ? 0.0f : static_cast<float>(*mixParam);

    // Update targets at block start
    driveSmoothed.setTargetValue(*driveParam);
    mixSmoothed.setTargetValue(targetMix);
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(*outputParam)));

    // Process each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        // Reset smoothed values to allow per-sample iteration for each channel
        // Note: We need to skip values for previous channels to stay in sync
        if (channel > 0)
        {
            // For stereo/multichannel: smoothed values advance per-sample
            // Reset to target to avoid desync between channels
            driveSmoothed.setCurrentAndTargetValue(driveSmoothed.getTargetValue());
            mixSmoothed.setCurrentAndTargetValue(mixSmoothed.getTargetValue());
            gainSmoothed.setCurrentAndTargetValue(gainSmoothed.getTargetValue());
        }

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Per-sample smoothing (NOT per-block!)
            float drive = driveSmoothed.getNextValue();
            float mix = mixSmoothed.getNextValue();
            float gain = gainSmoothed.getNextValue();

            float dry = channelData[sample];
            float wet = GrainDSP::applyWaveshaper(dry, drive);
            float mixed = GrainDSP::applyMix(dry, wet, mix);
            channelData[sample] = GrainDSP::applyGain(mixed, gain);
        }
    }
}

//==============================================================================
bool GRAINAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GRAINAudioProcessor::createEditor()
{
    return new GRAINAudioProcessorEditor (*this);
}

//==============================================================================
void GRAINAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save parameter state
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GRAINAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
