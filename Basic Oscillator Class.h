#pragma once

#include <cmath>

/**
 * The Phasor class serves as a base for various oscillator implementations. It tracks the phase of
 * the oscillator and provides functionality to update the phase and calculate the output based on
 * the current phase. It supports setting the sample rate, frequency, and initial phase.
 */

class Phasor {

public:

    float process()
    {
        phase += phaseDelta;

        if (phase > 1.0)
        {
            phase -= 1.0f;
        }
        return output(phase);
    }
    

    // Use this method to set the base frequency when a note starts
    void setBaseFrequency(float freq) {
        baseFrequency = freq;
        setFrequency(freq); // Set the initial frequency with no pitch bend
    }
    
    void setPhase (float _phase)
    {
        phase = _phase;
    }

    virtual float output(float p)
    {
        return p;
    }

    void setSampleRate(float SR)
    {
        sampleRate = SR;
    }

    void setFrequency(float freq)
    {
        frequency = freq;
        phaseDelta = frequency / sampleRate;
    }
    
    float getFrequency() const {
        return frequency;
    }

    
    void setOsc(float sr, float freq, float _phase = 0.0f)
    {
        setSampleRate(sr);
        setFrequency(freq);
        setPhase(_phase);
    }
   
    float getPhase()
    {
        return phase;
    }
    

private:
    float frequency;
    float sampleRate;
    float phase = 0.0f;
    float phaseDelta;
    std::atomic<float>* pitchBendRangeParam;
    float baseFrequency;
};


class TriOsc : public Phasor
{
    /**
     * TriOsc class represents a triangle wave oscillator. It overrides the output function
     * to generate a triangle wave based on the current phase.
     */
    
    float output(float p) override
    {
        return fabsf(p - 0.5f) - 0.25f;
    }
};

class SinOsc : public Phasor
{
    /**
     * SinOsc class represents a sine wave oscillator. It overrides the output function
     * to generate a sine wave based on the current phase.
     */
    
    float output(float p) override
    {
        return sin(p * 2.0 * 3.14159);
    }
};

class SquareOsc : public Phasor
{
    /**
     * SquareOsc class represents a square wave oscillator with variable pulse width.
     * It overrides the output function to generate a square wave based on the current phase
     * and pulse width.
     */
    
public:

    float output(float p) override
    {
        float outVal = 0.5;

        if (p > pulseWidth)
        {
            outVal = -0.5;
        }
        
        return outVal;
    }

    void setPulseWidth(float pw)
    {
        pulseWidth = pw;
    }
    
    /**
       * Initializes the square wave oscillator with the sample rate, frequency, phase, and pulse width.
       * @param sr The sample rate in Hz.
       * @param freq The frequency in Hz.
       * @param _phase (Optional) The initial phase.
       * @param pw (Optional) The pulse width.
       */
    
    void setOsc(float sr, float freq, float _phase = 0.0f, float pw = 0.5f) {
        setSampleRate(sr);
        setFrequency(freq);
        setPhase(_phase);
        setPulseWidth(pw); // Set the pulse width, default or specified
    }

private:
    float pulseWidth = 0.5f;
};


class ASDROsc : public Phasor {
public:
    /**
     * ASDROsc class represents an oscillator with an ADSR envelope applied to its output.
     * It overrides the output function to shape the oscillator output according to the ADSR parameters, to control volume.
     */
    
    float output(float p) override 
    {
        float outVal = 0; // Initialize output value

        // Attack phase
        if (p < attack) 
        {
            outVal = p / attack;
        }
        // Decay phase
        else if (p >= attack && p < (attack + decay)) 
        {
            outVal = 1.0f + (sustain - 1.0f) * ((p - attack) / decay);
        }
        // Sustain phase
        else if (p >= (attack + decay) && p < (attack + decay + sustain)) 
        {
            outVal = sustain;
        }
        // Release phase - needs to correctly calculate the slope for release
        else if (p >= (attack + decay + sustain) && p <= 1.0f) 
        {
            float releasePhaseStart = attack + decay + sustain;
            
            float releaseDuration = 1.0f - releasePhaseStart;
            
            outVal = sustain * (1.0f - (p - releasePhaseStart) / releaseDuration);
        }

        return outVal;
    }

    /**
       * Sets the attack time of the ADSR envelope.
       * @param _attack The attack time.
       */
    void setAttack(float _attack) 
    {
        attack = _attack;
    }

    /**
     * Sets the sustain level of the ADSR envelope.
     * @param _sustain The sustain level.
     */
    void setSustain(float _sustain) 
    {
        sustain = _sustain;
    }
    
    /**
     * Sets the decay time of the ADSR envelope.
     * @param _decay The decay time.
     */
    void setDecay(float _decay) {
        decay = _decay;
    }

    /**
       * Sets the release time of the ADSR envelope.
       * @param _release The release time.
       */
    
    void setRelease(float _release) {
        release = _release;
    }
    /**
       * Sets the ADSR envelope parameters.
       * @param a The attack time.
       * @param s The sustain level.
       * @param d The decay time.
       * @param r The release time.
       */
    void setASDR(float a, float s, float d, float r) {
        setAttack(a);
        setSustain(s);
        setDecay(d);
        setRelease(r);
    }
    
    /**
       * Initializes the oscillator with a preset ADSR shape, sample rate, frequency, and phase.
       * @param presetType The preset type ("Chord" or "Note").
       * @param sr The sample rate in Hz.
       * @param freq The frequency in Hz.
       * @param phase The initial phase.
       */
    
    void setShape(const std::string& presetType, float sr, float freq, float phase = 0.0f) {

        setOsc(sr, freq, phase);
        
        // Apply preset ADSR settings
        if (presetType == "Chord")
        {
            setASDR(0.004f, 0.02f, 0.04f, 0.001f);
        } 
        else if (presetType == "Note")
        
        {
            setASDR(0.0025f, 0.01f, 0.02f, 0.001f);
        }
    }


private:
    
    float attack;  // Attack time
    float decay;   // Decay time
    float sustain; // Sustain level
    float release; // Release time
};

