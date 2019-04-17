#include "Grain.h"
#include <math.h>

Grain::Grain(int size, int onset, float speed)
    : size(size), onset(onset), speed(speed), inactive(true)
{

}

Grain::~Grain()
{

}

void Grain::init()
{
    inactive = false; // Make this grain active so it cannot be used
}

void Grain::processSample(float *sampleBuffer, int sampleBufferLength, int readPointer, float* out)
{
    const float position = fmodf(((readPointer - onset) * speed), sampleBufferLength);
    const int posBelow = (int) std::floor(position);
    int posAbove = (int) std::ceil(position);
    if (posAbove >= sampleBufferLength) posAbove = 0;
    const float fracPos = position - (float) posBelow;

    float  currentSample = sampleBuffer[posBelow] * fracPos + (1 - fracPos) * sampleBuffer[posAbove];

    // // [3]
    // float currentSample = sampleBuffer[iPosition + startPosition % sampleBufferLength];
    // float a = sampleBuffer[(iPosition + startPosition) - 3 + sampleBufferLength % sampleBufferLength];
    // float b = sampleBuffer[(iPosition + startPosition) - 2 + sampleBufferLength % sampleBufferLength];
    // float c = sampleBuffer[(iPosition + startPosition) - 1 + sampleBufferLength % sampleBufferLength];
    //
    // currentSample = cubicinterp(fracPos, a, b, c, currentSample);

    float gain = this->window(readPointer - onset);
    *out += gain * currentSample;

    if (++readPointer - onset >= size) inactive = true;
}

float Grain::window(int phase)
{
    if (0 <= phase && phase < size / 2)
    {
        return 0.5 * (1 + cos(2 * M_PI / size * (phase - size / 2)));
    }
    else if (size - size / 2 <= phase && phase <= size)
    {
        return 0.5 * (1 + cos(2 * M_PI / size * (phase - 1 - size / 2)));
    }
    return 0;
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

float Grain::cubicinterp(float x, float y0, float y1, float y2, float y3)
{
    // 4-point, 3rd-order Hermite (x-form)
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * x + c2) * x + c1) * x + c0;
}
