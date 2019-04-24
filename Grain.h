class Grain
{
public:
    Grain(int size, int onset, float speed, float alpha);
    ~Grain();

    void processSample(float* sampleBuffer, int sampleBufferLength, float* out);
    float window();

    void setOnset(int onset);
    void setSpeed(float speed);
    void setSize(int size);
    void setTexture(float texture);

    void activate();
    bool isInactive();

    static float cubicinterp(float x, float y0, float y1, float y2, float y3);
private:
    int size;
    int onset;
    float phase;
    float speed;
    bool inactive;
    float texture;

    float winPhase;
};
