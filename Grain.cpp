#include "Grain.h"
#include <math.h>

Grain::Grain(int size, int onset, float speed, float texture)
    : size(size), onset(onset), phase(0), speed(speed), inactive(true), texture(texture), winPhase(0)
{

}

Grain::~Grain()
{

}

void Grain::processSample(float *sampleBuffer, int sampleBufferLength, float* out, float* window, int winLen)
{
    const float position = fmodf(((onset + phase) * speed), sampleBufferLength);
    // const int posBelow = (int) std::floor(position);
    // int posAbove = (int) std::ceil(position);
    // if (posAbove >= sampleBufferLength) posAbove = 0;
    // const float fracPos = position - (float) posBelow;
    //
    // const float currentSample = sampleBuffer[posBelow] * fracPos + (1 - fracPos) * sampleBuffer[posAbove];
    const float currentSample = linearInterp(sampleBuffer, sampleBufferLength, position);

    const float gain = linearInterp(window, winLen, winPhase);
    // float gain = this->window();
    *out += gain * currentSample;

    const float windowGrainRatio = (float) winLen / (float) size;
    winPhase += windowGrainRatio;
    if (winPhase >= winLen) winPhase -= winLen;

    if (++phase >= size)
    {
        inactive = true;
        phase = 0;
    }
}

float Grain::window()
{
    // if (0 <= phase && phase < size / 2)
    // {
    //     return 0.5f * (1 + cosf((float) 2 * M_PI / size * (phase - size / 2)));
    // }
    // else if (size - size / 2 <= phase && phase <= size)
    // {
    //     return 0.5f * (1 + cosf((float) 2 * M_PI / size * (phase - 1 - size / 2)));
    // }
    // return 0;
    if (0 <= phase && phase < floor(texture * size / 2.0f))
    {
        return 0.5f * (1 + cosf(M_PI * (((2.0f * phase) / (texture * size) - 1))));
    }
    else if (floor(texture * size / 2.0f) <= phase && phase <= size * (1 - texture / 2.0f))
    {
        return 1.0f;
    }
    else if (size * (1 - texture / 2.0f) < phase && phase < size)
    {
        return 0.5f * (1 + cosf(M_PI * (((2.0f * phase) / (texture * size) - (2.0f / texture) + 1))));
    }
    return 0.0f;
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

void Grain::setTexture(float texture)
{
    this->texture = texture;
}

bool Grain::isInactive()
{
    return inactive;
}

void Grain::activate()
{
    inactive = false;
}

float Grain::linearInterp(float *buffer, int bufferLength, float position)
{
    const int posBelow = (int) std::floor(position);
    int posAbove = (int) std::ceil(position);
    if (posAbove >= bufferLength) posAbove = 0;
    const float fracPos = position - (float) posBelow;

    return buffer[posBelow] * fracPos + (1.0f - fracPos) * buffer[posAbove];
}

float Grain::cubicinterp(float x, float y0, float y1, float y2, float y3)
{
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * x + c2) * x + c1) * x + c0;
}
