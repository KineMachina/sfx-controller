#ifndef BASS_MONO_PROCESSOR_H
#define BASS_MONO_PROCESSOR_H

#include <stdint.h>

/**
 * BassMonoProcessor - DSP processor for bass mono (low-end mono) mixing.
 *
 * Below a configurable crossover frequency, left and right channels are summed
 * to mono. Above the crossover, stereo imaging is preserved. This provides
 * coherent low-frequency energy for tactile transducers / subwoofers.
 *
 * Uses a 2nd-order Butterworth biquad low-pass filter (Direct Form II Transposed).
 * Pure math — no Arduino/ESP32 dependencies.
 */
class BassMonoProcessor {
public:
    BassMonoProcessor();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    /** Set crossover frequency in Hz. Clamped to 20–500 Hz. */
    void setCrossoverHz(uint16_t hz);
    uint16_t getCrossoverHz() const;

    /** Set sample rate and recalculate filter coefficients. */
    void setSampleRate(uint32_t rate);
    uint32_t getSampleRate() const;

    /**
     * Process interleaved stereo buffer in-place.
     * @param buff Interleaved [L0, R0, L1, R1, ...] samples
     * @param samplePairs Number of stereo sample pairs (half the buffer length)
     */
    void process(int16_t* buff, uint16_t samplePairs);

    /** Clear filter delay lines (call on track/rate change). */
    void reset();

private:
    void recalcCoefficients();

    bool enabled_;
    uint16_t crossoverHz_;
    uint32_t sampleRate_;

    // Biquad coefficients (2nd-order Butterworth LPF)
    float b0_, b1_, b2_;
    float a1_, a2_;

    // Direct Form II Transposed delay lines (one per channel)
    float w1L_, w2L_;
    float w1R_, w2R_;
};

#endif // BASS_MONO_PROCESSOR_H
