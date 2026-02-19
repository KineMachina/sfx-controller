#include "BassMonoProcessor.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BassMonoProcessor::BassMonoProcessor()
    : enabled_(false)
    , crossoverHz_(80)
    , sampleRate_(0)
    , b0_(0.0f), b1_(0.0f), b2_(0.0f)
    , a1_(0.0f), a2_(0.0f)
    , w1L_(0.0f), w2L_(0.0f)
    , w1R_(0.0f), w2R_(0.0f)
{
}

void BassMonoProcessor::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool BassMonoProcessor::isEnabled() const {
    return enabled_;
}

void BassMonoProcessor::setCrossoverHz(uint16_t hz) {
    if (hz < 20) hz = 20;
    if (hz > 500) hz = 500;
    crossoverHz_ = hz;
    if (sampleRate_ > 0) {
        recalcCoefficients();
    }
}

uint16_t BassMonoProcessor::getCrossoverHz() const {
    return crossoverHz_;
}

void BassMonoProcessor::setSampleRate(uint32_t rate) {
    if (rate == sampleRate_) return;
    sampleRate_ = rate;
    reset();
    if (rate > 0) {
        recalcCoefficients();
    }
}

uint32_t BassMonoProcessor::getSampleRate() const {
    return sampleRate_;
}

void BassMonoProcessor::recalcCoefficients() {
    if (sampleRate_ == 0) return;

    // Use double precision for trig, then cast to float for runtime
    double fc = (double)crossoverHz_;
    double fs = (double)sampleRate_;

    // Butterworth 2nd-order LPF via bilinear transform
    double wc = 2.0 * M_PI * fc / fs;
    double cosw = cos(wc);
    double sinw = sin(wc);
    // Q = 1/sqrt(2) for Butterworth
    double alpha = sinw / (2.0 * 0.7071067811865476);  // sqrt(2)/2

    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosw;
    double a2 = 1.0 - alpha;
    double b0 = (1.0 - cosw) / 2.0;
    double b1 = 1.0 - cosw;
    double b2 = (1.0 - cosw) / 2.0;

    // Normalize by a0
    b0_ = (float)(b0 / a0);
    b1_ = (float)(b1 / a0);
    b2_ = (float)(b2 / a0);
    a1_ = (float)(a1 / a0);
    a2_ = (float)(a2 / a0);
}

void BassMonoProcessor::process(int16_t* buff, uint16_t samplePairs) {
    if (!enabled_ || sampleRate_ == 0 || buff == nullptr || samplePairs == 0) {
        return;
    }

    for (uint16_t i = 0; i < samplePairs; i++) {
        float inL = (float)buff[i * 2];
        float inR = (float)buff[i * 2 + 1];

        // Direct Form II Transposed biquad — left channel
        float bassL = b0_ * inL + w1L_;
        w1L_ = b1_ * inL - a1_ * bassL + w2L_;
        w2L_ = b2_ * inL - a2_ * bassL;

        // Direct Form II Transposed biquad — right channel
        float bassR = b0_ * inR + w1R_;
        w1R_ = b1_ * inR - a1_ * bassR + w2R_;
        w2R_ = b2_ * inR - a2_ * bassR;

        // Mono bass = average of low-frequency content
        float monoBass = (bassL + bassR) * 0.5f;

        // Reconstruct: high-frequency stereo + mono bass
        float outL = (inL - bassL) + monoBass;
        float outR = (inR - bassR) + monoBass;

        // Clamp to int16_t range
        if (outL > 32767.0f) outL = 32767.0f;
        if (outL < -32768.0f) outL = -32768.0f;
        if (outR > 32767.0f) outR = 32767.0f;
        if (outR < -32768.0f) outR = -32768.0f;

        buff[i * 2]     = (int16_t)outL;
        buff[i * 2 + 1] = (int16_t)outR;
    }
}

void BassMonoProcessor::reset() {
    w1L_ = 0.0f;
    w2L_ = 0.0f;
    w1R_ = 0.0f;
    w2R_ = 0.0f;
}
