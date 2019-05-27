class Grain
{
public:
    Grain();
    Grain(int size, int onset, float speed);
    ~Grain();

    void processSample(float* sampleBuffer, int sampleBufferLength, float* out, float* window, int winLen);
    float window();

    void setOnset(int onset);
    void setSpeed(float speed);
    void setSize(int size);

    void activate();
    bool isInactive();

    static float linearInterpolate(float* sampleBuffer, int sampleBufferLength, float position);
private:
    int size;      // grain size in number of frame
    int onset;     // onset of the grain as a pointer into the audio sample buffer
    float phase;   // phase of the grain from its onset
    float speed;   // cycle rate of the grain
    bool inactive; // grain active state - can be scheduled if inactive
    float winPhase; // phase of this grain in the window buffer
};
