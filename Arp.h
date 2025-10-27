/*
  ==============================================================================

    Arp.h
    Created: 22 Apr 2024 2:12:38pm
    Author:  Caitlin Earley

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>

/**
 * A simple arpeggiator class that handles tempo synchronization with the host.
 * It manages the arpeggiation rate, note playback and integrates with the host's playback state.
 */
class Arpeggiator
{
public:
    
    /**
     * Sets the fallback beats per minute (BPM) if the host tempo is unavailable.
     * @param _fallbackBPM The fallback tempo in BPM.
     */
    void setFallbackBPM(double _fallbackBPM)
    {
        fallbackBPM = _fallbackBPM;
    }
    
    /**
     * Sets the playback head for retrieving the current playback state from the host.
     * @param _playHead Pointer to the host's AudioPlayHead.
     */
    void setPlayHead(juce::AudioPlayHead* _playHead)
    {
        playHead = _playHead;
    }
    
    /**
     * Sets the rate of the arpeggiator.
     * @param _rate Rate of arpeggiation, where 1: 1/4 notes, 2: 1/8 notes, 4: 1/16 notes, 8: 1/32 notes.
     */
    void setRate(int _rate)
    {
        rate = _rate;
    }
    
    /**
     * Retrieves the current position in quarter notes, considering the host's playback position.
     * @return Current position in quarter notes.
     */
    double getPositionInQuarterNotes()
    {
        if (playHead && playHead->getPosition() && playHead->getPosition()->getPpqPosition())
            return *playHead->getPosition()->getPpqPosition();
        
        return positionInQuarterNotes;
    }
    
    /**
     * Retrieves the current BPM from the host, or uses the fallback BPM if unavailable.
     * @return Current BPM.
     */
    double getBPM()
    {
        if (playHead && playHead->getPosition() && playHead->getPosition()->getBpm())
            return *playHead->getPosition()->getBpm();
        
        return fallbackBPM;
    }
    
    /**
     * Checks if the playback is currently active.
     * @return True if playing, false otherwise.
     */
    bool getIsPlaying()
    {
        if (playHead && playHead->getPosition() && playHead->getPosition()->getIsPlaying())
            return playHead->getPosition()->getIsPlaying();
        
        return true;
    }
    
    /**
     * Prepares the arpeggiator to play, initializing sample rates and note tracking.
     * @param _sampleRate The audio sample rate.
     */
    void prepareToPlay(double _sampleRate, int)
    {
        sampleRate = _sampleRate;
        notes.clear();
        noteIndex = 0;
        lastNote = -1;
    }
    
    /**
     * Processes the audio and MIDI data for the current audio block.
     * This method should be called in each cycle of the audio processing loop.
     * It calculates the timing for arpeggiation, handles MIDI note on/off messages, and triggers new notes when required.
     * @param buffer The buffer containing audio data.
     * @param midiMessages The MIDI buffer containing incoming and outgoing MIDI messages.
     */
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        auto numSamples = buffer.getNumSamples();  // Get the number of samples in the current audio buffer.
        
        positionInQuarterNotes = getPositionInQuarterNotes();  // Update current musical position in quarter notes.
        double bpm = getBPM();  // Retrieve the current tempo in beats per minute from the host.
        
        bool wasPlaying = isPlaying;  // Store the previous playing state.
        isPlaying = getIsPlaying();  // Update current playing state.
        
        // Check for transition from stop to start to reset the note index, preparing for a new arpeggiation sequence.
        if (!wasPlaying && isPlaying)
            noteIndex = 0;
        
        // Process all incoming MIDI messages to update note sets based on note on/off.
        for (const auto event : midiMessages)
        {
            const auto message = event.getMessage();
            auto noteNumber = message.getNoteNumber();
            
            if (message.isNoteOn())
                notes.add(noteNumber);  // Add note number to the set of currently held notes if note on.
            else if (message.isNoteOff())
                notes.removeValue(noteNumber);  // Remove note number from the set if note off.
        }
        
        // Calculate how many quarter notes occur per second, and how many samples represent one quarter note.
        double quarterNotesPerSecond = bpm / 60.0;
        double quarterNoteSamples = sampleRate / quarterNotesPerSecond;
        double samplesToNextNote = numSamples;  // Initialize with the buffer size (will calculate exact samples to next note).
        
        // Define potential positions in quarter notes where notes could be triggered, based on the current arpeggiation rate.
        double positions[] = {0.0, 0.125, 0.25, 0.375, 0.50, 0.625, 0.75, 0.875, 1.0};
        int positionCount = sizeof(positions) / sizeof(positions[0]);
        int skip = juce::jmax(1, (positionCount - 1) / rate);  // Calculate the skip in the positions array, based on rate.
        
        // Calculate the next note's position in terms of samples.
        for (int i = 0; i < positionCount; i += skip) {
            double pos = positions[i];
            double frac = positionInQuarterNotes - std::floor(positionInQuarterNotes);  // Fractional part of current position.
            double distanceToNextNote = pos - frac;
            samplesToNextNote = distanceToNextNote * quarterNoteSamples;
            
            // Break as soon as we find the next note position that is in the future.
            if (samplesToNextNote >= 0)
                break;
        }
        
        // Trigger new notes if we have reached the time to play the next note within this buffer.
        if (samplesToNextNote < numSamples)
        {
            // If there was a last note playing, send a note off message for it.
            if (lastNote != -1)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNote), samplesToNextNote);
                lastNote = -1;
            }
            
            // If there are notes held down, start the next note.
            if (notes.size() > 0)
            {
                lastNote = notes[noteIndex];  // Get the next note to play.
                noteIndex = (noteIndex + 1) % notes.size();  // Advance to the next note in the set.
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, lastNote, juce::uint8(127)), samplesToNextNote);
            }
        }
        
        // Increment the musical position by the number of samples processed divided by samples per quarter note.
        positionInQuarterNotes += numSamples / quarterNoteSamples;
    }

    
private:
    double sampleRate;
    double positionInQuarterNotes;
    int rate = 1;
    int noteIndex = 0;
    int lastNote = -1;
    bool isPlaying;
    double fallbackBPM;
    juce::AudioPlayHead* playHead;
    juce::SortedSet<int> notes;
};
