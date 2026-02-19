#include <unity.h>
#include "MatrixMath.h"
#include <cmath>

// 16x16 grid constants used throughout tests
static const int W = 16;
static const int H = 16;
static const int N = W * H; // 256

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// xyToIndex — even rows (left-to-right)
// ---------------------------------------------------------------------------

void test_xy_origin(void) {
    TEST_ASSERT_EQUAL_INT(0, MatrixMath::xyToIndex(0, 0, W, H));
}

void test_xy_end_of_first_row(void) {
    TEST_ASSERT_EQUAL_INT(15, MatrixMath::xyToIndex(15, 0, W, H));
}

void test_xy_even_row_mid(void) {
    // Row 2 (even): left-to-right, starts at index 32
    TEST_ASSERT_EQUAL_INT(37, MatrixMath::xyToIndex(5, 2, W, H));
}

// ---------------------------------------------------------------------------
// xyToIndex — odd rows (right-to-left, serpentine)
// ---------------------------------------------------------------------------

void test_xy_odd_row_leftmost(void) {
    // Row 1 (odd): right-to-left. x=0 maps to end of row → index 31
    TEST_ASSERT_EQUAL_INT(31, MatrixMath::xyToIndex(0, 1, W, H));
}

void test_xy_odd_row_rightmost(void) {
    // Row 1 (odd): x=15 maps to start of row → index 16
    TEST_ASSERT_EQUAL_INT(16, MatrixMath::xyToIndex(15, 1, W, H));
}

void test_xy_odd_row_mid(void) {
    // Row 3 (odd): starts at 48, x=5 → 48 + (15 - 5) = 58
    TEST_ASSERT_EQUAL_INT(58, MatrixMath::xyToIndex(5, 3, W, H));
}

// ---------------------------------------------------------------------------
// xyToIndex — boundaries
// ---------------------------------------------------------------------------

void test_xy_last_pixel(void) {
    // Row 15 (odd): x=0 → 15*16 + (16-1-0) = 240 + 15 = 255
    TEST_ASSERT_EQUAL_INT(255, MatrixMath::xyToIndex(0, 15, W, H));
}

void test_xy_last_row_rightmost(void) {
    // Row 15 (odd): x=15 → 15*16 + (16-1-15) = 240
    TEST_ASSERT_EQUAL_INT(240, MatrixMath::xyToIndex(15, 15, W, H));
}

// ---------------------------------------------------------------------------
// xyToIndex — out of bounds
// ---------------------------------------------------------------------------

void test_xy_negative_x(void) {
    TEST_ASSERT_EQUAL_INT(-1, MatrixMath::xyToIndex(-1, 0, W, H));
}

void test_xy_negative_y(void) {
    TEST_ASSERT_EQUAL_INT(-1, MatrixMath::xyToIndex(0, -1, W, H));
}

void test_xy_x_too_large(void) {
    TEST_ASSERT_EQUAL_INT(-1, MatrixMath::xyToIndex(16, 0, W, H));
}

void test_xy_y_too_large(void) {
    TEST_ASSERT_EQUAL_INT(-1, MatrixMath::xyToIndex(0, 16, W, H));
}

// ---------------------------------------------------------------------------
// xyToIndex — different grid sizes
// ---------------------------------------------------------------------------

void test_xy_8x8_grid(void) {
    // 8x8 grid, row 1 (odd), x=0 → 8 + (8-1-0) = 15
    TEST_ASSERT_EQUAL_INT(15, MatrixMath::xyToIndex(0, 1, 8, 8));
}

void test_xy_non_square_grid(void) {
    // 10x5 grid, row 0, x=9 → 9
    TEST_ASSERT_EQUAL_INT(9, MatrixMath::xyToIndex(9, 0, 10, 5));
    // row 1 (odd), x=0 → 10 + (10-1-0) = 19
    TEST_ASSERT_EQUAL_INT(19, MatrixMath::xyToIndex(0, 1, 10, 5));
}

// ---------------------------------------------------------------------------
// indexToXY — even rows
// ---------------------------------------------------------------------------

void test_index_origin(void) {
    int x, y;
    MatrixMath::indexToXY(0, x, y, W, N);
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(0, y);
}

void test_index_end_first_row(void) {
    int x, y;
    MatrixMath::indexToXY(15, x, y, W, N);
    TEST_ASSERT_EQUAL_INT(15, x);
    TEST_ASSERT_EQUAL_INT(0, y);
}

// ---------------------------------------------------------------------------
// indexToXY — odd rows (serpentine reversal)
// ---------------------------------------------------------------------------

void test_index_start_odd_row(void) {
    // Index 16 is start of row 1 storage, which is x=15 (rightmost) in serpentine
    int x, y;
    MatrixMath::indexToXY(16, x, y, W, N);
    TEST_ASSERT_EQUAL_INT(15, x);
    TEST_ASSERT_EQUAL_INT(1, y);
}

void test_index_end_odd_row(void) {
    // Index 31 is end of row 1 storage, which is x=0 (leftmost) in serpentine
    int x, y;
    MatrixMath::indexToXY(31, x, y, W, N);
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(1, y);
}

// ---------------------------------------------------------------------------
// indexToXY — out of bounds
// ---------------------------------------------------------------------------

void test_index_negative(void) {
    int x, y;
    MatrixMath::indexToXY(-1, x, y, W, N);
    TEST_ASSERT_EQUAL_INT(-1, x);
    TEST_ASSERT_EQUAL_INT(-1, y);
}

void test_index_too_large(void) {
    int x, y;
    MatrixMath::indexToXY(256, x, y, W, N);
    TEST_ASSERT_EQUAL_INT(-1, x);
    TEST_ASSERT_EQUAL_INT(-1, y);
}

// ---------------------------------------------------------------------------
// xyToIndex / indexToXY — roundtrip consistency
// ---------------------------------------------------------------------------

void test_roundtrip_all_pixels(void) {
    // Every index should roundtrip: indexToXY → xyToIndex = original index
    for (int i = 0; i < N; i++) {
        int x, y;
        MatrixMath::indexToXY(i, x, y, W, N);
        TEST_ASSERT_TRUE_MESSAGE(x >= 0 && x < W, "x out of range after indexToXY");
        TEST_ASSERT_TRUE_MESSAGE(y >= 0 && y < H, "y out of range after indexToXY");
        int idx = MatrixMath::xyToIndex(x, y, W, H);
        TEST_ASSERT_EQUAL_INT_MESSAGE(i, idx, "Roundtrip indexToXY→xyToIndex failed");
    }
}

void test_roundtrip_xy_to_index(void) {
    // Every valid (x,y) should roundtrip: xyToIndex → indexToXY = original (x,y)
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int idx = MatrixMath::xyToIndex(x, y, W, H);
            TEST_ASSERT_TRUE_MESSAGE(idx >= 0 && idx < N, "index out of range after xyToIndex");
            int rx, ry;
            MatrixMath::indexToXY(idx, rx, ry, W, N);
            TEST_ASSERT_EQUAL_INT_MESSAGE(x, rx, "x mismatch in roundtrip");
            TEST_ASSERT_EQUAL_INT_MESSAGE(y, ry, "y mismatch in roundtrip");
        }
    }
}

// ---------------------------------------------------------------------------
// distance2D
// ---------------------------------------------------------------------------

void test_distance_zero(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, MatrixMath::distance2D(0, 0, 0, 0));
}

void test_distance_3_4_5(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, MatrixMath::distance2D(0, 0, 3, 4));
}

void test_distance_horizontal(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, MatrixMath::distance2D(1, 5, 8, 5));
}

void test_distance_vertical(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, MatrixMath::distance2D(3, 2, 3, 12));
}

void test_distance_symmetric(void) {
    float d1 = MatrixMath::distance2D(1, 2, 5, 8);
    float d2 = MatrixMath::distance2D(5, 8, 1, 2);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, d1, d2);
}

// ---------------------------------------------------------------------------
// perlinNoise2D — range and determinism
// ---------------------------------------------------------------------------

void test_perlin_in_range(void) {
    // Sample many points and verify output is in [0, 1]
    for (float t = 0.0f; t < 10.0f; t += 1.0f) {
        for (float x = 0.0f; x < 5.0f; x += 0.5f) {
            for (float y = 0.0f; y < 5.0f; y += 0.5f) {
                float v = MatrixMath::perlinNoise2D(x, y, t);
                TEST_ASSERT_TRUE_MESSAGE(v >= 0.0f && v <= 1.0f,
                    "perlinNoise2D output out of [0,1] range");
            }
        }
    }
}

void test_perlin_deterministic(void) {
    float v1 = MatrixMath::perlinNoise2D(1.5f, 2.3f, 0.7f);
    float v2 = MatrixMath::perlinNoise2D(1.5f, 2.3f, 0.7f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, v1, v2);
}

void test_perlin_varies_with_input(void) {
    float v1 = MatrixMath::perlinNoise2D(0.0f, 0.0f, 0.0f);
    float v2 = MatrixMath::perlinNoise2D(3.0f, 3.0f, 5.0f);
    // Different inputs should (very likely) produce different values
    TEST_ASSERT_TRUE(fabsf(v1 - v2) > 0.001f);
}

// ---------------------------------------------------------------------------
// getCharBitmap — known characters
// ---------------------------------------------------------------------------

void test_char_space_is_blank(void) {
    for (int row = 0; row < 7; row++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, MatrixMath::getCharBitmap(' ', row));
    }
}

void test_char_G_top_row(void) {
    // 'G' row 0 = 0x0E (01110)
    TEST_ASSERT_EQUAL_HEX8(0x0E, MatrixMath::getCharBitmap('G', 0));
}

void test_char_E_top_row(void) {
    // 'E' row 0 = 0x1F (11111)
    TEST_ASSERT_EQUAL_HEX8(0x1F, MatrixMath::getCharBitmap('E', 0));
}

void test_char_case_insensitive(void) {
    for (int row = 0; row < 7; row++) {
        TEST_ASSERT_EQUAL_UINT8(
            MatrixMath::getCharBitmap('G', row),
            MatrixMath::getCharBitmap('g', row));
        TEST_ASSERT_EQUAL_UINT8(
            MatrixMath::getCharBitmap('A', row),
            MatrixMath::getCharBitmap('a', row));
    }
}

void test_char_unknown_returns_zero(void) {
    for (int row = 0; row < 7; row++) {
        TEST_ASSERT_EQUAL_UINT8(0, MatrixMath::getCharBitmap('Z', row));
        TEST_ASSERT_EQUAL_UINT8(0, MatrixMath::getCharBitmap('1', row));
    }
}

void test_char_row_out_of_range(void) {
    TEST_ASSERT_EQUAL_UINT8(0, MatrixMath::getCharBitmap('G', -1));
    TEST_ASSERT_EQUAL_UINT8(0, MatrixMath::getCharBitmap('G', 7));
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // xyToIndex — even rows
    RUN_TEST(test_xy_origin);
    RUN_TEST(test_xy_end_of_first_row);
    RUN_TEST(test_xy_even_row_mid);

    // xyToIndex — odd rows
    RUN_TEST(test_xy_odd_row_leftmost);
    RUN_TEST(test_xy_odd_row_rightmost);
    RUN_TEST(test_xy_odd_row_mid);

    // xyToIndex — boundaries
    RUN_TEST(test_xy_last_pixel);
    RUN_TEST(test_xy_last_row_rightmost);

    // xyToIndex — out of bounds
    RUN_TEST(test_xy_negative_x);
    RUN_TEST(test_xy_negative_y);
    RUN_TEST(test_xy_x_too_large);
    RUN_TEST(test_xy_y_too_large);

    // xyToIndex — different grid sizes
    RUN_TEST(test_xy_8x8_grid);
    RUN_TEST(test_xy_non_square_grid);

    // indexToXY — even rows
    RUN_TEST(test_index_origin);
    RUN_TEST(test_index_end_first_row);

    // indexToXY — odd rows
    RUN_TEST(test_index_start_odd_row);
    RUN_TEST(test_index_end_odd_row);

    // indexToXY — out of bounds
    RUN_TEST(test_index_negative);
    RUN_TEST(test_index_too_large);

    // Roundtrip
    RUN_TEST(test_roundtrip_all_pixels);
    RUN_TEST(test_roundtrip_xy_to_index);

    // distance2D
    RUN_TEST(test_distance_zero);
    RUN_TEST(test_distance_3_4_5);
    RUN_TEST(test_distance_horizontal);
    RUN_TEST(test_distance_vertical);
    RUN_TEST(test_distance_symmetric);

    // perlinNoise2D
    RUN_TEST(test_perlin_in_range);
    RUN_TEST(test_perlin_deterministic);
    RUN_TEST(test_perlin_varies_with_input);

    // getCharBitmap
    RUN_TEST(test_char_space_is_blank);
    RUN_TEST(test_char_G_top_row);
    RUN_TEST(test_char_E_top_row);
    RUN_TEST(test_char_case_insensitive);
    RUN_TEST(test_char_unknown_returns_zero);
    RUN_TEST(test_char_row_out_of_range);

    return UNITY_END();
}
