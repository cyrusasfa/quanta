//
//  Compressor.cpp
//  MultibandCompressor - VST3
//
//  Based on example compression plugin in JUCE by Reiss and McPherson
//
//  Limiter
//  Audio Effects Theory and Implementation, Chapter 6: Dynamics Processing
//  by Joshua Reiss, Andrew McPherson, Brecht de Man
//  https://code.soundsoftware.ac.uk/projects/audio_effects_textbook_code/repository/show/effects/compressor/Source
//  ---

#pragma once

#include <stdio.h>

class Compressor
{
public:
    Compressor();
    ~Compressor();

    void prepareToPlay (double sampleRate);
    void processSample (float* sample);
private:
    // parameters
    float ratio, threshold, tauAttack, tauRelease, alphaAttack, alphaRelease;

    float inputGain, inputLevel, outputGain, outputLevel, control, previousOutputLevel;
    int sampleRate;
};
