//
//  Compressor.cpp
//  MultibandCompressor - VST3
//
//  Based on example compression plugin in JUCE by Reiss and McPherson
//
//  Dynamic range compression
//  Audio Effects Theory and Implementation, Chapter 6: Dynamics Processing
//  by Joshua Reiss, Andrew McPherson, Brecht de Man
//  https://code.soundsoftware.ac.uk/projects/audio_effects_textbook_code/repository/show/effects/compressor/Source
//  ---

#include "Compressor.h"
#include <math.h>

Compressor::Compressor()
    : ratio(50), threshold(-6), tauAttack(10), tauRelease(60)
{
}

Compressor::~Compressor()
{
}

//==============================================================================
void Compressor::prepareToPlay (double sampleRate)
{
    this->sampleRate = sampleRate;
    previousOutputLevel = 0;
}

void Compressor::processSample (float* sample)
{
    alphaAttack  = exp(-1/(0.001 * sampleRate * tauAttack));
    alphaRelease = exp(-1/(0.001 * sampleRate * tauRelease));
    // apply control voltage to the audio signal
    //Level detection- estimate level using peak detector
    if (fabs(*sample) < 0.000001)
    {
        inputGain = -120;
    } else {
        inputGain = 20 * log10(fabs(*sample));
    }
    //Gain computer- static apply input/output curve
    if (inputGain >= threshold) {
        outputGain = threshold + (inputGain - threshold) / ratio;
    } else {
        outputGain = inputGain;
    }
    inputLevel = inputGain - outputGain;
    //Ballistics- smoothing of the gain
    if (inputLevel > previousOutputLevel) {
        outputLevel = alphaAttack * previousOutputLevel + (1 - alphaAttack) * inputLevel;
    }
    else {
        outputLevel = alphaRelease * previousOutputLevel + (1 - alphaRelease) * inputLevel;
    }
    // find control voltage
    control = powf(15.0f, (0 - outputLevel) * 0.05f);
    previousOutputLevel = outputLevel;

    *sample *= control;
}
