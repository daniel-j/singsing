#pragma once

// Based on https://github.com/JorenSix/Pidato/blob/master/libraries/Yin/Yin.h

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

    void initialize(float sampleRate, int bufferSize);
    float getPitch(const float *buffer);
    float getProbability();

private:
    float parabolicInterpolation(int tauEstimate);
    int absoluteThreshold();
    void cumulativeMeanNormalizedDifference();
    void difference(const float *buffer);
};

