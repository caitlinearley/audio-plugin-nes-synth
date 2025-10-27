/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Arp.h"
#include "DrumSampler.h"

//==============================================================================
SynthExampleAudioProcessor::SynthExampleAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    //add voices to synth
    synth.addSound(new BitCrusherSound());
    for (int i = 0; i < voiceCount; i++) {
        synth.addVoice(new BitCrusherVoice());
    }

    // Set up each voice to use the parameters from APVTS
    for (int i = 0; i < synth.getNumVoices(); i++) {
        if (auto* voice = dynamic_cast<BitCrusherVoice*>(synth.getVoice(i))) {
            voice->setParametersFromAPVTS(apvts);
        }
    }
    
    // add voices to sampler
    for (int i =0; i<voiceCount; i++)
    {
        sampler.addVoice(new juce::SamplerVoice());
        
    }
    //add each sample to a specific note
    sampler.setSample(BinaryData::Bongo_01_wav, BinaryData::Bongo_01_wavSize, 53, 53);
    sampler.setSample(BinaryData::clap_wav, BinaryData::clap_wavSize, 55, 55);
    sampler.setSample(BinaryData::tom_wav, BinaryData::tom_wavSize, 57, 57);
    sampler.setSample(BinaryData::kick_wav, BinaryData::kick_wavSize, 59, 59);
}

SynthExampleAudioProcessor::~SynthExampleAudioProcessor()
{
}

//==============================================================================
const juce::String SynthExampleAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SynthExampleAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SynthExampleAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SynthExampleAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SynthExampleAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SynthExampleAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SynthExampleAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SynthExampleAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SynthExampleAudioProcessor::getProgramName (int index)
{
    return {};
}

void SynthExampleAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SynthExampleAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    arpeggiator.prepareToPlay(sampleRate, samplesPerBlock);

    synth.setCurrentPlaybackSampleRate(sampleRate);
    sampler.setCurrentPlaybackSampleRate(sampleRate);
    reverb.reset();
    reverb.setSampleRate(sampleRate);
}

void SynthExampleAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SynthExampleAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SynthExampleAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Clear the audio buffer
    buffer.clear();

    // Set up the arpeggiator parameters
    arpeggiator.setFallbackBPM(80.0);
    arpeggiator.setPlayHead(getPlayHead());
    arpeggiator.setRate(*apvts.getRawParameterValue("arpRate"));

    // Process arpeggiator if enabled
    if (apvts.getRawParameterValue("arpEnabled")->load() == 0)
    {
        arpeggiator.processBlock(buffer, midiMessages);
    }

    // Render next block for the synthesizer
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    // Process sampler if mode is 2 (sampler mode and white noise) and arpeggiator is off
    if (apvts.getRawParameterValue("mode")->load() == 2)
    {
        sampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
        // Apply bit-crushing to buffer
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float currentSample = buffer.getSample(0, sample);
            
            //dynamic cast functions so that we can use it from synth class
            auto voice = dynamic_cast<BitCrusherVoice*>(synth.getVoice(0));
            
            float bitcrushedSample = voice->bitcrushing(currentSample, 0);
            
            left[sample] = bitcrushedSample;
            right[sample] = bitcrushedSample;
        }
    }
    
    // Process reverb if enabled
    if(apvts.getRawParameterValue("reverbToggle")->load())
    {
        // Set up reverb parameters
        juce::Reverb::Parameters reverbParams;
        reverbParams.dryLevel = apvts.getRawParameterValue("reverbDry")->load();
        reverbParams.wetLevel = apvts.getRawParameterValue("reverbWet")->load();
        reverbParams.roomSize = apvts.getRawParameterValue("reverbRoomSize")->load();
        reverb.setParameters(reverbParams);
        
        // Apply reverb to the buffer
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        reverb.processStereo(left, right, buffer.getNumSamples());
    }

    // Clear the MIDI messages buffer
    midiMessages.clear();
}


//==============================================================================
bool SynthExampleAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SynthExampleAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void SynthExampleAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    // this code goes in getStateInformation()
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SynthExampleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SynthExampleAudioProcessor();
}
