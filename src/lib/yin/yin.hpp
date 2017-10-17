#pragma once

class Yin {
private:
    double threshold;
    int bufferSize;
    int halfBufferSize;
    float sampleRate;
    float* yinBuffer = nullptr;
    float probability;
public:
    Yin();
    Yin(float sampleRate, int bufferSize);
    ~Yin();

    void initialize(float sampleRate, int bufferSize, float yinThreshold = 0.15);

    float getPitch(const float *buffer);
    float getProbability();

private:
    void difference(const float *buffer);
    void cumulativeMeanNormalizedDifference();
    int absoluteThreshold();
    float parabolicInterpolation(int tauEstimate);
};
