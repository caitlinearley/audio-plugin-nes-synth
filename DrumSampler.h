/*
  ==============================================================================

    Sampler.h
    Created: 4 Apr 2024 06:45:22am
    Author:  Cautlin

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Sampler : public juce::Synthesiser
{
public:
    
    Sampler(){
        
        formatManager.registerBasicFormats();
    }
    
    void setSample(const void* sourceData, size_t sourceDataSize, int startMidiNote, int endMidiNote)
    {
        // Allows us to use WAV and AIFF files
        
        
        // Create a MemoryInputStream for the source data
        std::unique_ptr<juce::MemoryInputStream> inputStream(new juce::MemoryInputStream(sourceData, sourceDataSize, false));

        // Create an AudioFormatReader for the MemoryInputStream
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(inputStream)));
        
        // Create a BigInteger with the specified range of MIDI notes
        juce::BigInteger noteRange;
        noteRange.setRange(startMidiNote, endMidiNote - startMidiNote + 1, true);

        // Add the sample with the specified MIDI note range
        addSound(new juce::SamplerSound("default", *reader, noteRange, startMidiNote, 0, 0.1, 10));
    }

    
    
private:

    juce::AudioFormatManager formatManager;
    
};

