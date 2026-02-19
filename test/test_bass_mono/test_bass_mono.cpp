#include <unity.h>
#include "BassMonoProcessor.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static BassMonoProcessor* proc;

void setUp(void) {
    proc = new BassMonoProcessor();
}

void tearDown(void) {
    delete proc;
    proc = nullptr;
}

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

void test_default_disabled(void) {
    TEST_ASSERT_FALSE(proc->isEnabled());
}

void test_default_crossover(void) {
    TEST_ASSERT_EQUAL_UINT16(80, proc->getCrossoverHz());
}

void test_default_sample_rate(void) {
    TEST_ASSERT_EQUAL_UINT32(0, proc->getSampleRate());
}

// ---------------------------------------------------------------------------
// Setters / getters
// ---------------------------------------------------------------------------

void test_set_enabled(void) {
    proc->setEnabled(true);
    TEST_ASSERT_TRUE(proc->isEnabled());
    proc->setEnabled(false);
    TEST_ASSERT_FALSE(proc->isEnabled());
}

void test_set_crossover(void) {
    proc->setCrossoverHz(120);
    TEST_ASSERT_EQUAL_UINT16(120, proc->getCrossoverHz());
}

void test_crossover_clamp_low(void) {
    proc->setCrossoverHz(5);
    TEST_ASSERT_EQUAL_UINT16(20, proc->getCrossoverHz());
}

void test_crossover_clamp_high(void) {
    proc->setCrossoverHz(1000);
    TEST_ASSERT_EQUAL_UINT16(500, proc->getCrossoverHz());
}

void test_set_sample_rate(void) {
    proc->setSampleRate(44100);
    TEST_ASSERT_EQUAL_UINT32(44100, proc->getSampleRate());
}

// ---------------------------------------------------------------------------
// Process when disabled = buffer unchanged
// ---------------------------------------------------------------------------

void test_disabled_passthrough(void) {
    int16_t buf[] = {1000, -2000, 3000, -4000};
    int16_t orig[4];
    memcpy(orig, buf, sizeof(buf));

    proc->setSampleRate(44100);
    // enabled_ is false by default
    proc->process(buf, 2);

    TEST_ASSERT_EQUAL_INT16_ARRAY(orig, buf, 4);
}

// ---------------------------------------------------------------------------
// Process with null/zero args = no crash
// ---------------------------------------------------------------------------

void test_null_buffer(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->process(nullptr, 10);  // should not crash
}

void test_zero_pairs(void) {
    int16_t buf[] = {100, 200};
    int16_t orig[] = {100, 200};
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->process(buf, 0);
    TEST_ASSERT_EQUAL_INT16_ARRAY(orig, buf, 2);
}

// ---------------------------------------------------------------------------
// Mono input passes through unmodified (L == R at all frequencies)
// ---------------------------------------------------------------------------

void test_mono_input_unchanged(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->setCrossoverHz(80);

    // Generate a mono signal (L == R) — DC value
    const int N = 512;
    int16_t buf[N * 2];
    for (int i = 0; i < N; i++) {
        buf[i * 2]     = 5000;
        buf[i * 2 + 1] = 5000;
    }

    proc->process(buf, N);

    // After filter settles (~50 samples), L should equal R
    for (int i = 100; i < N; i++) {
        TEST_ASSERT_EQUAL_INT16(buf[i * 2], buf[i * 2 + 1]);
    }
}

// ---------------------------------------------------------------------------
// DC signal passes through after filter settles
// ---------------------------------------------------------------------------

void test_dc_passthrough(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->setCrossoverHz(80);

    const int N = 2048;
    int16_t buf[N * 2];
    for (int i = 0; i < N; i++) {
        buf[i * 2]     = 10000;
        buf[i * 2 + 1] = 10000;
    }

    proc->process(buf, N);

    // After settling, output should be very close to input DC value
    // (LPF passes DC, so bassL ≈ 10000, monoBass ≈ 10000, outL ≈ (10000-10000)+10000 = 10000)
    int16_t lastL = buf[(N - 1) * 2];
    int16_t lastR = buf[(N - 1) * 2 + 1];
    TEST_ASSERT_INT16_WITHIN(50, 10000, lastL);
    TEST_ASSERT_INT16_WITHIN(50, 10000, lastR);
}

// ---------------------------------------------------------------------------
// High-frequency stereo preserved above crossover
// ---------------------------------------------------------------------------

void test_high_freq_stereo_preserved(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->setCrossoverHz(80);

    // Generate a high-frequency stereo signal (1kHz, well above 80 Hz crossover)
    // L channel: sine at 1kHz, R channel: -sine at 1kHz (opposite phase)
    const int N = 1024;
    int16_t buf[N * 2];
    for (int i = 0; i < N; i++) {
        double t = (double)i / 44100.0;
        double val = 10000.0 * sin(2.0 * M_PI * 1000.0 * t);
        buf[i * 2]     = (int16_t)val;
        buf[i * 2 + 1] = (int16_t)(-val);
    }

    // Save a copy to compare
    int16_t orig[N * 2];
    memcpy(orig, buf, sizeof(buf));

    proc->process(buf, N);

    // High-frequency content should be largely preserved — check that L and R
    // still have opposite signs (stereo maintained) after settling
    int stereoCount = 0;
    for (int i = 200; i < N; i++) {
        if ((buf[i * 2] > 100 && buf[i * 2 + 1] < -100) ||
            (buf[i * 2] < -100 && buf[i * 2 + 1] > 100)) {
            stereoCount++;
        }
    }
    // At least 70% of samples should maintain stereo separation
    TEST_ASSERT_GREATER_THAN(((N - 200) * 70) / 100, stereoCount);
}

// ---------------------------------------------------------------------------
// Low-frequency stereo collapsed to mono below crossover
// ---------------------------------------------------------------------------

void test_low_freq_collapsed_to_mono(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->setCrossoverHz(200);  // Higher crossover for easier testing

    // Generate a low-frequency signal in LEFT channel only, RIGHT is silent.
    // Bass mono should redistribute bass energy to both channels.
    const int N = 8820;  // 0.2 seconds
    int16_t* buf = new int16_t[N * 2];
    for (int i = 0; i < N; i++) {
        double t = (double)i / 44100.0;
        double val = 15000.0 * sin(2.0 * M_PI * 50.0 * t);
        buf[i * 2]     = (int16_t)val;
        buf[i * 2 + 1] = 0;
    }

    proc->process(buf, N);

    // After settling, the RIGHT channel should have gained bass energy
    // (monoBass = bassL/2 is added to R). Check the last quarter.
    int startCheck = N * 3 / 4;
    int rightHasBass = 0;
    for (int i = startCheck; i < N; i++) {
        // R channel should no longer be silent — it should have received bass
        if (abs(buf[i * 2 + 1]) > 500) {
            rightHasBass++;
        }
    }
    int checkSamples = N - startCheck;
    // At least 60% of samples in the right channel should have audible bass
    TEST_ASSERT_GREATER_THAN((checkSamples * 60) / 100, rightHasBass);

    // Also verify L and R are closer together than the original L-only signal
    // Original spread: |L - 0| = up to 15000. After processing, L and R should
    // be much closer since bass is shared.
    int closeCount = 0;
    for (int i = startCheck; i < N; i++) {
        int diff = abs(buf[i * 2] - buf[i * 2 + 1]);
        if (diff < 5000) {
            closeCount++;
        }
    }
    TEST_ASSERT_GREATER_THAN((checkSamples * 50) / 100, closeCount);
    delete[] buf;
}

// ---------------------------------------------------------------------------
// Clamp to int16_t range (no overflow)
// ---------------------------------------------------------------------------

void test_no_overflow(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->setCrossoverHz(80);

    // Feed maximum amplitude values
    const int N = 256;
    int16_t buf[N * 2];
    for (int i = 0; i < N; i++) {
        buf[i * 2]     = 32767;
        buf[i * 2 + 1] = 32767;
    }

    proc->process(buf, N);

    // No sample should exceed int16_t range
    for (int i = 0; i < N; i++) {
        TEST_ASSERT_GREATER_OR_EQUAL(-32768, (int)buf[i * 2]);
        TEST_ASSERT_LESS_OR_EQUAL(32767, (int)buf[i * 2]);
        TEST_ASSERT_GREATER_OR_EQUAL(-32768, (int)buf[i * 2 + 1]);
        TEST_ASSERT_LESS_OR_EQUAL(32767, (int)buf[i * 2 + 1]);
    }
}

// ---------------------------------------------------------------------------
// Coefficient recalculation on sample rate change
// ---------------------------------------------------------------------------

void test_sample_rate_change_recalculates(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);

    // Process some data at 44100
    int16_t buf1[] = {10000, -10000, 10000, -10000};
    proc->process(buf1, 2);
    int16_t val44_L = buf1[2];  // save processed value

    // Change sample rate and process same input
    proc->setSampleRate(22050);
    int16_t buf2[] = {10000, -10000, 10000, -10000};
    proc->process(buf2, 2);
    int16_t val22_L = buf2[2];

    // Different sample rates should produce different filter results
    // (since the normalized frequency changes)
    TEST_ASSERT_NOT_EQUAL(val44_L, val22_L);
}

// ---------------------------------------------------------------------------
// reset() clears delay line state
// ---------------------------------------------------------------------------

void test_reset_clears_state(void) {
    proc->setEnabled(true);
    proc->setSampleRate(44100);
    proc->setCrossoverHz(80);

    // Process some data to build up filter state
    int16_t buf1[128];
    for (int i = 0; i < 64; i++) {
        buf1[i * 2]     = (int16_t)(10000 * sin(2.0 * M_PI * 40.0 * i / 44100.0));
        buf1[i * 2 + 1] = (int16_t)(-10000 * sin(2.0 * M_PI * 40.0 * i / 44100.0));
    }
    proc->process(buf1, 64);

    // Reset and process fresh
    proc->reset();
    int16_t buf2[4] = {5000, -5000, 5000, -5000};
    proc->process(buf2, 2);

    // Also process from a fresh instance
    BassMonoProcessor fresh;
    fresh.setEnabled(true);
    fresh.setSampleRate(44100);
    fresh.setCrossoverHz(80);
    int16_t buf3[4] = {5000, -5000, 5000, -5000};
    fresh.process(buf3, 2);

    // After reset, output should match a fresh processor
    TEST_ASSERT_EQUAL_INT16(buf3[0], buf2[0]);
    TEST_ASSERT_EQUAL_INT16(buf3[1], buf2[1]);
    TEST_ASSERT_EQUAL_INT16(buf3[2], buf2[2]);
    TEST_ASSERT_EQUAL_INT16(buf3[3], buf2[3]);
}

// ---------------------------------------------------------------------------
// Process without sample rate set = no-op
// ---------------------------------------------------------------------------

void test_no_sample_rate_passthrough(void) {
    proc->setEnabled(true);
    // sampleRate_ is 0 by default
    int16_t buf[] = {1000, -2000, 3000, -4000};
    int16_t orig[4];
    memcpy(orig, buf, sizeof(buf));
    proc->process(buf, 2);
    TEST_ASSERT_EQUAL_INT16_ARRAY(orig, buf, 4);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // Default state
    RUN_TEST(test_default_disabled);
    RUN_TEST(test_default_crossover);
    RUN_TEST(test_default_sample_rate);

    // Setters / getters
    RUN_TEST(test_set_enabled);
    RUN_TEST(test_set_crossover);
    RUN_TEST(test_crossover_clamp_low);
    RUN_TEST(test_crossover_clamp_high);
    RUN_TEST(test_set_sample_rate);

    // Process safety
    RUN_TEST(test_disabled_passthrough);
    RUN_TEST(test_null_buffer);
    RUN_TEST(test_zero_pairs);
    RUN_TEST(test_no_sample_rate_passthrough);

    // Signal processing behavior
    RUN_TEST(test_mono_input_unchanged);
    RUN_TEST(test_dc_passthrough);
    RUN_TEST(test_high_freq_stereo_preserved);
    RUN_TEST(test_low_freq_collapsed_to_mono);
    RUN_TEST(test_no_overflow);

    // State management
    RUN_TEST(test_sample_rate_change_recalculates);
    RUN_TEST(test_reset_clears_state);

    return UNITY_END();
}
