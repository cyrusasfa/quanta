#include "Grain.h"
#include <math.h>

Grain::Grain()
{

}

Grain::Grain(int size, int onset, float speed)
    : size(size), onset(onset), phase(0), speed(speed), inactive(true), winPhase(0)
{

}

Grain::~Grain()
{

}

void Grain::processSample(float *sampleBuffer, int sampleBufferLength, float* out, float* window, int winLen)
{
    if (!inactive) {
        const float position = fmodf(((onset + phase) * speed), sampleBufferLength); // Get fractional pointer for grain phase in the audio buffer
        const float currentSample = linearInterpolate(sampleBuffer, sampleBufferLength, position); // interpolate the pointer

        const float gain = linearInterpolate(window, winLen, winPhase); // interpolate a value from the window buffer using the grain's window phase
        *out += gain * currentSample; // apply window gian to current grain sample and accumulate in the global output

        const float windowGrainRatio = (float) winLen / (float) size;  // calculate window length to grain length ratio
        winPhase += windowGrainRatio; // increment pointer into the window table by the ratio between lengths
        if (winPhase >= winLen) winPhase -= winLen; // wrap window phase

        // handle grain duration elapsed
        if (++phase >= size)
        {
            // Deactivate the grain and reset phase params
            inactive = true;
            phase = 0;
            winPhase = 0;
        }
    }
} 

void Grain::setOnset(int onset)
{
    this->onset = onset;
}

void Grain::setSpeed(float speed)
{
    this->speed = speed;
}

void Grain::setSize(int size)
{
    this->size = size;
}

bool Grain::isInactive()
{
    return inactive;
}

void Grain::activate()
{
    inactive = false;
}

// Return an interpolated sample from a buffer given a fractional read position
float Grain::linearInterpolate(float *buffer, int bufferLength, float position)
{
    const int posBelow = (int) std::floor(position); // ptr below position
    int posAbove = (int) std::ceil(position);        // ptr above position
    if (posAbove >= bufferLength) posAbove = 0;      // wrap posAbove around circ buffer
    const float fracPos = position - (float) posBelow; // get fraction part

    // linearly interpolate discrete samples
    return buffer[posBelow] * fracPos + (1.0f - fracPos) * buffer[posAbove];
}
