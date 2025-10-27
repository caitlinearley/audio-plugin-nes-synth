/*
 ==============================================================================
   YourSynthesiser.h
   Created: 7 Mar 2020 4:27:57pm
   Author:  Caitlin Earley

 ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Basic Oscillator Class.h"


// ===========================
// ===========================
// SOUND
class BitCrusherSound : public juce::SynthesiserSound
{
public:
   bool appliesToNote      (int) override      { return true; }
   //--------------------------------------------------------------------------
   bool appliesToChannel   (int) override      { return true; }
};

// =================================
// =================================
// Synthesiser Voice - your synth code goes in here

/*!
@class YourSynthVoice
@abstract struct defining the DSP associated with a specific voice.
@discussion multiple YourSynthVoice objects will be created by the Synthesiser so that it can be played polyphicially

@namespace none
@updated 2019-06-18
*/
class BitCrusherVoice : public juce::SynthesiserVoice
{
public:
    BitCrusherVoice() {
        
        sinLFO.setSampleRate(getSampleRate());
        triLFO.setSampleRate(getSampleRate());
        squareLFO.setSampleRate(getSampleRate());
        pulse1.setSampleRate(getSampleRate());
        pulse2.setSampleRate(getSampleRate());
        bass.setSampleRate(getSampleRate());
        
        juce::ADSR::Parameters snareParams;
        snareParams.attack = 0.01;  // Very fast attack
        snareParams.decay = 0.15;   // Quick decay
        snareParams.sustain = 0.0;  // No sustain
        snareParams.release = 0.01; // Quick release
        envSnare.setParameters(snareParams);


        juce::ADSR::Parameters hatParams;
        hatParams.attack = 0.01;  // Very fast attack
        hatParams.decay = 0.08;   // Extremely quick decay
        hatParams.sustain = 0.0;  // No sustain
        hatParams.release = 0.01; // Quick release
        envHat.setParameters(hatParams);
        
        
        // Setup high-hat filter as a high-pass filter

        highHatFilter.setCoefficients(juce::IIRCoefficients::makeHighPass(getSampleRate(), 7000));
        highHatFilter.reset();

        // Setup snare filter as a band-pass filter
        snareFilter.setCoefficients(juce::IIRCoefficients::makeBandPass(getSampleRate(), 2000, 1));
        snareFilter.reset();
        

        
        
    }
    
    //--------------------------------------------------------------------------
    /**
     What should be done when a note starts
     
     @param midiNoteNumber
     @param velocity
     @param SynthesiserSound unused variable
     @param / unused variable
     */
    
    void setParametersFromAPVTS(juce::AudioProcessorValueTreeState& apvts) {
        
        attackParam = apvts.getRawParameterValue("attack");
        decayParam = apvts.getRawParameterValue("decay");
        sustainParam = apvts.getRawParameterValue("sustain");
        releaseParam = apvts.getRawParameterValue("release");
        
        bitDepth = apvts.getRawParameterValue("bitDepth");
        rateDivide = apvts.getRawParameterValue("rateDivide");
        noiseAmount = apvts.getRawParameterValue("noiseAmount");
        
        pulseWidth1Choice = apvts.getRawParameterValue("pulseWidth1");
        pulseWidth2Choice = apvts.getRawParameterValue("pulseWidth2");
        pitchOffset = apvts.getRawParameterValue("pitchOffset");
        
        modeParam = apvts.getRawParameterValue("mode");
        
        bitDepthLFOAmount = apvts.getRawParameterValue("bitDepthLFOAmount");
        LFORate = apvts.getRawParameterValue("LFORate");
        typeLFO = apvts.getRawParameterValue("typeLFO");
        
    }
    
    
    /**
     * Starts a note.
     * @param midiNoteNumber The MIDI note number of the note to start.
     * @param velocity The velocity of the note.
     * @param sound Pointer to the SynthesiserSound object.
     * @param currentPitchWheelPosition Unused parameter.
     */
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        playing = true;
        
        // set and update pulse width params
        
        float pulseWidth1Percent = (*pulseWidth1Choice == 0) ? 0.125f : ((*pulseWidth1Choice == 1) ? 0.25f : 0.5f);
        float pulseWidth2Percent = (*pulseWidth2Choice == 0) ? 0.125f : ((*pulseWidth2Choice == 1) ? 0.25f : 0.5f);
        
        float freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        float pitchOffsetFactor = std::pow(2.0f, (*pitchOffset / 12.0f));  // Calculate pitch offset
        
        
        //set all env params
        env.setSampleRate(getSampleRate());
        juce::ADSR::Parameters envParams;
        envParams.attack = *attackParam;
        envParams.decay = *decayParam;
        envParams.sustain = *sustainParam;
        envParams.release = *releaseParam;
        env.setParameters(envParams);
        env.noteOn();
        envHat.noteOn();
        envSnare.noteOn();
        
        
        
        
        //allow for pitch off set and set pulse widths
        if (modeParam->load() == 0) {
            
            bass.setFrequency(freq * pitchOffsetFactor);
        } else {
            
            pulse1.setFrequency(freq * pitchOffsetFactor);
            pulse2.setFrequency(freq * pitchOffsetFactor);
            
            pulse1.setPulseWidth(pulseWidth1Percent);
            pulse2.setPulseWidth(pulseWidth2Percent);
        }
        
        //store note
        currentMidi = midiNoteNumber;
    }
    
    /**
     * Stops a note.
     * @param velocity The velocity of the note.
     * @param allowTailOff Determines if the note should fade out or stop abruptly.
     */
    void stopNote(float velocity, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            env.noteOff();
            envSnare.noteOff();
            envHat.noteOff();
        } 
        else
        {
            // Immediately stops the note if tail-off is not allowed
            clearCurrentNote();
            playing = false;
        }
    }
    /**
     * Renders the next block of audio samples.
     * @param outputBuffer The buffer to render the audio samples into.
     * @param startSample The starting sample index.
     * @param numSamples The number of samples to render.
     */
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (playing) 
        {
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); ++sampleIndex)
            {
                float currentEnvSample = env.getNextSample();
                // see if env is in attck phase
                bool inAttackPhase = currentEnvSample > lastEnvSample;
                
                lastEnvSample = currentEnvSample;
                
                float outputSample = 0.0f;

                // LFO and bit depth processing
                float lfoValue = 0.0f;
                if (typeLFO->load() == 0) //sin LFO
                {
                    sinLFO.setFrequency(*LFORate);
                    lfoValue = sinLFO.process();
                } 
                else if (typeLFO->load() == 1)
                {
                    triLFO.setFrequency(*LFORate); //tri LFO
                    lfoValue = triLFO.process();
                } 
                else if (typeLFO->load() == 2)
                
                {
                    squareLFO.setFrequency(*LFORate); //square LFO
                    squareLFO.setPulseWidth(0.5);
                    lfoValue = squareLFO.process();
                }

                // Processing based on modeParam
                if (modeParam->load() == 2) // process white noise for noise/drum channel
                {
                    // Noise processing for specific MIDI notes
                    if (currentMidi == 60) 
                    { // Hi-Hat Noise
                        float rawSample = (random.nextFloat() * 2.0f - 1.0f) * envHat.getNextSample();;  // Hat noise with specific envelope
                        outputSample = highHatFilter.processSingleSampleRaw(rawSample);//apply filter
                    }
                    else if (currentMidi == 62)
                    { // Snare Noise
                        float rawSample = (random.nextFloat() * 2.0f - 1.0f) * envSnare.getNextSample(); // Snare noise with specific envelope
                        outputSample = snareFilter.processSingleSampleRaw(rawSample); //apply filter
                    }
                    else if (currentMidi == 64)
                    {
                        // White noise for all other keys
                        outputSample = (random.nextFloat() * 2.0f - 1.0f) * currentEnvSample;
                    }
                    else 
                    {
                        continue;
                    }
                } else {
                    // Regular synthesizer sound processing
                    if (modeParam->load() == 0) //process bass
                    {
                        outputSample = bass.process() * currentEnvSample;
                    } 
                    else if (modeParam->load() == 1) //process pulses
                    {
                        bool pulseWidthsAreEqual = (*pulseWidth1Choice == *pulseWidth2Choice);
                        
                        if (pulseWidthsAreEqual)
                        {
                            outputSample = pulse1.process() * currentEnvSample;
                        } 
                        else
                        {
                            //this allows us to switch between cycle dutys if the pulse withs are different
                            if (inAttackPhase)
                            {
                                //play first pulse for attack
                                outputSample = pulse1.process() * currentEnvSample;
                            } 
                            else
                            {
                                //change to second pulse for remiander of note
                                outputSample = pulse2.process() * currentEnvSample;
                            }
                        }
                    }
                }

                if (!(modeParam->load() == 2)) //do not bit crush white noise as this gets done later
                {
                    outputSample = bitcrushing(outputSample, lfoValue);
                }
                // Sample rate division processing
                if (sampleIndex % static_cast<int>(*rateDivide) != 0) 
                {
                    outputSample = outputBuffer.getSample(0, sampleIndex - sampleIndex % static_cast<int>(*rateDivide));
                }

                for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) 
                {
                    outputBuffer.addSample(channel, sampleIndex, outputSample);
                }

                //stop playing when env value is no longer active
                if (!env.isActive()) 
                {
                    playing = false;
                    
                    clearCurrentNote();
                }
            }
        }
    }

    /**
     * Applies the bitcrushing effect to the input sample.
     * @param sample The input sample to be processed.
     * @param lfoValue The value of the LFO used for modulating bit depth.
     * @return The bitcrushed output sample.
     */
    float bitcrushing(float sample, float lfoValue)
    {
        // Calculate the modulated bit depth using the LFO value
        float modulatedBitDepth = *bitDepth + lfoValue * (*bitDepthLFOAmount);
        
        // Ensure the modulated bit depth stays within a valid range
        modulatedBitDepth = juce::jlimit(1.0f, 24.0f, modulatedBitDepth);
        
        // Calculate the bit depth power
        float bitDepthPow = powf(2.0f, modulatedBitDepth) - 1.0f;
        
        // Apply bitcrushing and normalize the output sample
        return floorf(sample * bitDepthPow + 0.5f) / bitDepthPow;
    }

    
    
    void pitchWheelMoved(int newValue) override {
        // No pitch wheel functionality needed
    }
    
    //--------------------------------------------------------------------------
    void controllerMoved(int, int) override {}
    //--------------------------------------------------------------------------
    /**
     Can this voice play a sound. I wouldn't worry about this for the time being
     
     @param sound a juce::SynthesiserSound* base class pointer
     @return sound cast as a pointer to an instance of YourSynthSound
     */
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<BitCrusherSound*> (sound) != nullptr;
    }
    //--------------------------------------------------------------------------
private:
    //--------------------------------------------------------------------------
    // Set up any necessary variables here
    /// Should the voice be playing?
    bool playing = false;
    
    float lastEnvSample = 0.0f;
    
    
    juce::IIRFilter highHatFilter;
    juce::IIRFilter snareFilter;
    
    
    SinOsc sinLFO;
    TriOsc bass, triLFO;
    SquareOsc pulse1, pulse2, squareLFO;
    
    /// a random object for use in our test noise function
    juce::Random random;
    juce::ADSR env, envHat, envSnare;
    
    std::atomic<float>* noiseAmount;
    std::atomic<float>* bitDepth;
    std::atomic<float>* rateDivide;
    
    std::atomic<float>* attackParam;
    std::atomic<float>* decayParam;
    std::atomic<float>* sustainParam;
    std::atomic<float>* releaseParam;
    
    
    std::atomic<float>* delayVolumeParam;
    std::atomic<float>* pitchOffset;
    
    std::atomic<float>* pulseWidth1Choice;
    std::atomic<float>* pulseWidth2Choice;
    std::atomic<float>* modeParam;
    
    std::atomic<float>* bitDepthLFOAmount;
    std::atomic<float>* LFORate;
    std::atomic <float>* typeLFO;
    
    float lfoValue;
    int currentMidi;
};
