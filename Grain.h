class Grain
{
public:
    Grain(int size, int onset, float speed);
    ~Grain();

    void init();
    void processSample(float* sampleBuffer, int sampleBufferLength, int readPointer, float* out);
    float window(int phase);

    void setOnset(int onset);
    void setSpeed(float speed);
    void setSize(int size);

    void activate();
    bool isInactive();

    static float cubicinterp(float x, float y0, float y1, float y2, float y3);
private:
    int size;
    int onset;
    float speed;
    bool inactive;
};
