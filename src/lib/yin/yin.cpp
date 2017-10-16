#include "yin.hpp"

// Based on https://github.com/JorenSix/Pidato/blob/master/libraries/Yin/Yin.cpp
// and https://github.com/ashokfernandez/Yin-Pitch-Tracking/blob/master/Yin.c
// License: GPL

Yin::Yin() {}
Yin::Yin(float yinSampleRate,int yinBufferSize) {
    initialize(yinSampleRate, yinBufferSize);
}
Yin::~Yin() {
    if (yinBuffer) delete[] yinBuffer;
}

void Yin::initialize(float yinSampleRate, int yinBufferSize, float yinThreshold) {
    bufferSize = yinBufferSize;
    sampleRate = yinSampleRate;
    halfBufferSize = bufferSize / 2;
    threshold = yinThreshold;
    probability = 0.0;
    // initialize array and set it to zero
    if (yinBuffer) delete[] yinBuffer;
    yinBuffer = new float[halfBufferSize]();
}

float Yin::getProbability() {
    return probability;
}

float Yin::getPitch(const float* buffer) {
    int tauEstimate = -1;
    float pitchInHertz = -1;

    /* Step 2: Calculates the squared difference of the signal with a shifted version of itself. */
    difference(buffer);

    /* Step 3: Calculate the cumulative mean on the normalised difference calculated in step 1 */
    cumulativeMeanNormalizedDifference();

    /* Step 4: Search through the normalised cumulative mean array and find values that are over the threshold */
    tauEstimate = absoluteThreshold();

    /* Step 5: Interpolate the shift value (tau) to improve the pitch estimate. */
    if (tauEstimate != -1) {
        pitchInHertz = sampleRate / parabolicInterpolation(tauEstimate);
    }

    return pitchInHertz;
}
/**
 * Step 2: Calculates the squared difference of the signal with a shifted version of itself.
 *
 * This is the Yin algorithms tweak on autocorellation. Read http://audition.ens.fr/adc/pdf/2002_JASA_YIN.pdf
 * for more details on what is in here and why it's done this way.
 */
void Yin::difference(const float* buffer) {
    int index, tau;
    float delta;

    /* Calculate the difference for difference shift values (tau) for the half of the samples */
    for (tau = 0 ; tau < halfBufferSize; tau++) {

        /* Take the difference of the signal with a shifted version of itself, then square it.
         * (This is the Yin algorithm's tweak on autocorellation) */
        for (index = 0; index < halfBufferSize; index++) {
            delta = buffer[index] - buffer[index + tau];
            yinBuffer[tau] += delta * delta;
        }
    }
}

/**
 * Step 3: Calculate the cumulative mean on the normalised difference calculated in step 1
 *
 * This goes through the Yin autocorellation values and finds out roughly where shift is which
 * produced the smallest difference
 */
void Yin::cumulativeMeanNormalizedDifference() {
    int tau;
    float runningSum = 0;
    yinBuffer[0] = 1;

    /* Sum all the values in the autocorellation buffer and nomalise the result, replacing
     * the value in the autocorellation buffer with a cumulative mean of the normalised difference */
    for (tau = 1; tau < halfBufferSize; tau++) {
        runningSum += yinBuffer[tau];
        yinBuffer[tau] *= tau / runningSum;
    }
}

/**
 * Step 4: Search through the normalised cumulative mean array and find values that are over the threshold
 * @return Shift (tau) which caused the best approximate autocorellation. -1 if no suitable value is found over the threshold.
 */
int Yin::absoluteThreshold() {
    int tau;

    /* Search through the array of cumulative mean values, and look for ones that are over the threshold
     * The first two positions in yinBuffer are always 1 so start at the third (index 2) */
    for (tau = 2; tau < halfBufferSize ; tau++) {
        if (yinBuffer[tau] < threshold) {
            while (tau + 1 < halfBufferSize && yinBuffer[tau + 1] < yinBuffer[tau]) {
                tau++;
            }
            // found tau, exit loop and return
            // store the probability
            // From the YIN paper: The threshold determines the list of
            // candidates admitted to the set, and can be interpreted as the
            // proportion of aperiodic power tolerated
            // within a periodic signal.
            //
            // Since we want the periodicity and and not aperiodicity:
            // periodicity = 1 - aperiodicity
            probability = 1 - yinBuffer[tau];
            break;
        }
    }
    // if no pitch found, tau => -1
    if (tau == halfBufferSize || yinBuffer[tau] >= threshold) {
        tau = -1;
        probability = 0;
    }
    return tau;
}

/**
 * Step 5: Interpolate the shift value (tau) to improve the pitch estimate.
 *
 * The 'best' shift value for autocorellation is most likely not an interger shift of the signal.
 * As we only autocorellated using integer shifts we should check that there isn't a better fractional
 * shift value.
 */
float Yin::parabolicInterpolation(int tauEstimate) {
    float betterTau;
    int x1 = tauEstimate;
    int x2 = tauEstimate;

    /* Calculate the first polynomial coeffcient based on the current estimate of tau */
    if (tauEstimate < 1) {
        x1 = tauEstimate;
    } else {
        x1 = tauEstimate - 1;
    }
    // if (tauEstimate >= 1) x1 = tauEstimate - 1;

    /* Calculate the second polynomial coeffcient based on the current estimate of tau */
    if (tauEstimate + 1 < halfBufferSize) {
        x2 = tauEstimate + 1;
    } else {
        x2 = tauEstimate;
    }
    // if (tauEstimate + 1 < halfBufferSize) x2 = tauEstimate + 1;

    /* Algorithm to parabolically interpolate the shift value tau to find a better estimate */
    if (x1 == tauEstimate) {
        if (yinBuffer[tauEstimate] <= yinBuffer[x2]) {
            betterTau = tauEstimate;
        } else {
            betterTau = x2;
        }
    } else if (x2 == tauEstimate) {
        if (yinBuffer[tauEstimate] <= yinBuffer[x1]) {
            betterTau = tauEstimate;
        } else {
            betterTau = x1;
        }
    } else {
        float s0, s1, s2;
        s0 = yinBuffer[x1];
        s1 = yinBuffer[tauEstimate];
        s2 = yinBuffer[x2];
        // fixed AUBIO implementation, thanks to Karl Helgason:
        // (2.0f * s1 - s2 - s0) was incorrectly multiplied with -1
        betterTau = tauEstimate + (s2 - s0) / (2 * (2 * s1 - s2 - s0));
    }
    return betterTau;
}
