/*
  ==============================================================================

    Sample.h
    Created: 19 Apr 2024 3:22:18pm
    Author:  Caitlin  Earley

  ==============================================================================
*/


#pragma once
#include <JuceHeader.h>

class Sampler : public juce::Synthesiser
{
public:
    
    Sampler(){}
    
    void setSample(const void* sourceData, size_t sourceDataSize, int midiNoteNumber)
    {
        
        // This allows us to use WAV and AIFF files
        formatManager.registerBasicFormats();
        
        // Here we create a MemoryInputStream for the source data
        std::unique_ptr<juce::MemoryInputStream> inputStream(new juce::MemoryInputStream(sourceData, sourceDataSize, false));

        // Create an AudioFormatReader for the MemoryInputStream
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(inputStream)));
        
        juce::BigInteger allNotes;
        allNotes.setRange(midiNoteNumber, 1, true);
        
        // arguments:
        //  name
        //  reference to reader
        //  BigInteger - what MIDI notes it can play
        //  center note - chosen 60 = middle C
        //  attack time - 0 as samples should hopefully have curated fade in
        //  release - 0.1 to try avoid clicks
        
        //  maximum sample time
        addSound( new juce::SamplerSound("default", *reader, allNotes, 120, 0, 0.1, 10) );
    }
    
private:

    juce::AudioFormatManager formatManager;
    
};

