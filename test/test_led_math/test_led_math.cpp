#include <unity.h>
#include "LEDMath.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// packColor / unpack helpers
// ---------------------------------------------------------------------------

void test_pack_black(void) {
    TEST_ASSERT_EQUAL_HEX32(0x000000, LEDMath::packColor(0, 0, 0));
}

void test_pack_white(void) {
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF, LEDMath::packColor(255, 255, 255));
}

void test_pack_red(void) {
    TEST_ASSERT_EQUAL_HEX32(0xFF0000, LEDMath::packColor(255, 0, 0));
}

void test_pack_green(void) {
    TEST_ASSERT_EQUAL_HEX32(0x00FF00, LEDMath::packColor(0, 255, 0));
}

void test_pack_blue(void) {
    TEST_ASSERT_EQUAL_HEX32(0x0000FF, LEDMath::packColor(0, 0, 255));
}

void test_unpack_red(void) {
    TEST_ASSERT_EQUAL_UINT8(0xAB, LEDMath::unpackR(0xAB1234));
}

void test_unpack_green(void) {
    TEST_ASSERT_EQUAL_UINT8(0x12, LEDMath::unpackG(0xAB1234));
}

void test_unpack_blue(void) {
    TEST_ASSERT_EQUAL_UINT8(0x34, LEDMath::unpackB(0xAB1234));
}

void test_pack_unpack_roundtrip(void) {
    uint8_t r = 42, g = 128, b = 200;
    uint32_t c = LEDMath::packColor(r, g, b);
    TEST_ASSERT_EQUAL_UINT8(r, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(g, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(b, LEDMath::unpackB(c));
}

// ---------------------------------------------------------------------------
// colorWheel — full HSV-like color cycle
// ---------------------------------------------------------------------------

void test_wheel_position_0(void) {
    // wheelPos=0: inverted to 255, third segment: (255*3=765 overflows, but 255-170=85)
    // wheelPos=255-0=255, >=170: wp-=170→85, Color(85*3,255-85*3,0) = (255,0,0)
    uint32_t c = LEDMath::colorWheel(0);
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackB(c));
}

void test_wheel_position_85(void) {
    // wheelPos=85: inverted to 170, exactly at boundary of segment 2
    // 255-85=170, >=170: wp-=170→0, Color(0,255,0) = green
    uint32_t c = LEDMath::colorWheel(85);
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackB(c));
}

void test_wheel_position_170(void) {
    // wheelPos=170: inverted to 85, exactly at boundary of segment 2
    // 255-170=85, >=85 <170: wp-=85→0, Color(0,0,255) = blue
    uint32_t c = LEDMath::colorWheel(170);
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::unpackB(c));
}

void test_wheel_all_positions_nonzero(void) {
    // Every position should produce a non-black color
    for (int i = 0; i < 256; i++) {
        uint32_t c = LEDMath::colorWheel((uint8_t)i);
        TEST_ASSERT_TRUE_MESSAGE(c != 0, "colorWheel produced black");
    }
}

void test_wheel_component_sum_255(void) {
    // For all positions, exactly two components are nonzero and they sum to 255
    for (int i = 0; i < 256; i++) {
        uint32_t c = LEDMath::colorWheel((uint8_t)i);
        uint8_t r = LEDMath::unpackR(c);
        uint8_t g = LEDMath::unpackG(c);
        uint8_t b = LEDMath::unpackB(c);
        int sum = r + g + b;
        TEST_ASSERT_EQUAL_INT_MESSAGE(255, sum,
            "colorWheel RGB components should sum to 255");
    }
}

// ---------------------------------------------------------------------------
// blendColor — linear interpolation
// ---------------------------------------------------------------------------

void test_blend_ratio_0(void) {
    // ratio=0: 100% color1
    uint32_t c = LEDMath::blendColor(0xFF0000, 0x0000FF, 0.0f);
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackB(c));
}

void test_blend_ratio_1(void) {
    // ratio=1: 100% color2
    uint32_t c = LEDMath::blendColor(0xFF0000, 0x0000FF, 1.0f);
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::unpackB(c));
}

void test_blend_midpoint(void) {
    // ratio=0.5: 50/50 blend of red and blue
    uint32_t c = LEDMath::blendColor(0xFE0000, 0x0000FE, 0.5f);
    TEST_ASSERT_EQUAL_UINT8(127, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(127, LEDMath::unpackB(c));
}

void test_blend_clamp_negative(void) {
    // ratio < 0 should clamp to 0
    uint32_t c = LEDMath::blendColor(0xFF0000, 0x0000FF, -0.5f);
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackB(c));
}

void test_blend_clamp_above_1(void) {
    // ratio > 1 should clamp to 1
    uint32_t c = LEDMath::blendColor(0xFF0000, 0x0000FF, 1.5f);
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::unpackB(c));
}

void test_blend_same_color(void) {
    uint32_t c = LEDMath::blendColor(0x804020, 0x804020, 0.5f);
    TEST_ASSERT_EQUAL_HEX32(0x804020, c);
}

void test_blend_black_to_white(void) {
    uint32_t c = LEDMath::blendColor(0x000000, 0xFEFEFE, 0.5f);
    TEST_ASSERT_EQUAL_UINT8(127, LEDMath::unpackR(c));
    TEST_ASSERT_EQUAL_UINT8(127, LEDMath::unpackG(c));
    TEST_ASSERT_EQUAL_UINT8(127, LEDMath::unpackB(c));
}

// ---------------------------------------------------------------------------
// fadeChannel — per-channel fade
// ---------------------------------------------------------------------------

void test_fade_full_brightness(void) {
    // fadeValue=256 should keep color unchanged (256/256 = 1.0 but >>8 truncates)
    // Actually (255 * 256) >> 8 = 255, so full brightness preserved
    TEST_ASSERT_EQUAL_UINT8(255, LEDMath::fadeChannel(255, 256));
}

void test_fade_half_brightness(void) {
    // fadeValue=128: (200 * 128) >> 8 = 100
    TEST_ASSERT_EQUAL_UINT8(100, LEDMath::fadeChannel(200, 128));
}

void test_fade_to_zero(void) {
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::fadeChannel(255, 0));
}

void test_fade_already_zero(void) {
    TEST_ASSERT_EQUAL_UINT8(0, LEDMath::fadeChannel(0, 128));
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // packColor / unpack
    RUN_TEST(test_pack_black);
    RUN_TEST(test_pack_white);
    RUN_TEST(test_pack_red);
    RUN_TEST(test_pack_green);
    RUN_TEST(test_pack_blue);
    RUN_TEST(test_unpack_red);
    RUN_TEST(test_unpack_green);
    RUN_TEST(test_unpack_blue);
    RUN_TEST(test_pack_unpack_roundtrip);

    // colorWheel
    RUN_TEST(test_wheel_position_0);
    RUN_TEST(test_wheel_position_85);
    RUN_TEST(test_wheel_position_170);
    RUN_TEST(test_wheel_all_positions_nonzero);
    RUN_TEST(test_wheel_component_sum_255);

    // blendColor
    RUN_TEST(test_blend_ratio_0);
    RUN_TEST(test_blend_ratio_1);
    RUN_TEST(test_blend_midpoint);
    RUN_TEST(test_blend_clamp_negative);
    RUN_TEST(test_blend_clamp_above_1);
    RUN_TEST(test_blend_same_color);
    RUN_TEST(test_blend_black_to_white);

    // fadeChannel
    RUN_TEST(test_fade_full_brightness);
    RUN_TEST(test_fade_half_brightness);
    RUN_TEST(test_fade_to_zero);
    RUN_TEST(test_fade_already_zero);

    return UNITY_END();
}
