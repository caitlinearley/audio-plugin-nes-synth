/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Synthesiser Starting code (sound and voice).h"
#include "DrumSampler.h"
#include "Arp.h"

//==============================================================================
/**
*/
class SynthExampleAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SynthExampleAudioProcessor();
    ~SynthExampleAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:

    // create objects
    juce::Reverb reverb;

    juce::Synthesiser synth;

    Arpeggiator arpeggiator;
    
    Sampler sampler;
    
    //number of voices
    int voiceCount = 16;

    // param tree
    juce::AudioProcessorValueTreeState apvts;

    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        
        //choose channel
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("mode", 1),"Type", juce::StringArray{"Bass", "Pulse", "Noise/Drum"}, 1));
            
        // env params
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("attack", 1), "Attack", 0.001, 1.0, 0.01));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("decay", 1), "Decay", 0.001, 1.0, 0.25));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sustain", 1), "Sustain", 0.001, 1.0, 0.5));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("release", 1), "Release", 0.001, 1.0, 1.0));

        // duty cycles
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("pulseWidth1", 1), "Pulse Width 1", juce::StringArray{"12.5%", "25%", "50%"}, 0));
            
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("pulseWidth2", 1), "Pulse Width 2", juce::StringArray{"12.5%", "25%", "50%"}, 0));

        //pitch offset
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pitchOffset", 1),"Pitch Offset", -12.0, 12.0, 0.0));
        
        //turn arp on or off
            
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("arpEnabled", 1),"Arp Switch", juce::StringArray{"On", "Off"}, 1));
        //set arpeggio rate
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("arpRate", 1),"Arp Rate", 1.0, 8.0, 1.0));
            
        // bit crushing and down sampling params
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID("rateDivide", 1), "dwsr", 1, 10, 1));
            
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("bitDepth", 1),"Bit Depth", 1, 32, 32));
        
        //type, rate and depth of lfo to control bit crushing modulation
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("typeLFO", 1),"LFO Type", juce::StringArray{"Square", "Sine", "Triangle"}, 1));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("bitDepthLFOAmount", 1),"LFO Modulation", 0, 10, 1));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LFORate", 1),"LFO Rate", 0.01, 10, 0.5));
            
        // Reverb toggle
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("reverbToggle", 1), "Reverb toggle", false));
        // Reverb parameters
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("reverbDry", 1),"Reverb dry level", 0.01f, 1, 0.01f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("reverbWet", 1),"Reverb wet level", 0.01f, 1, 0.01f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("reverbRoomSize", 1),"Reverb room size", 0, 1, 0));

        return layout;
    }
    


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthExampleAudioProcessor)
};
