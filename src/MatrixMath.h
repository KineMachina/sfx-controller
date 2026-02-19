#ifndef MATRIX_MATH_H
#define MATRIX_MATH_H

#include <stdint.h>
#include <math.h>

namespace MatrixMath {

// Convert (x, y) grid coordinates to linear LED index for serpentine wiring.
// Even rows: left-to-right. Odd rows: right-to-left.
// Returns -1 for out-of-bounds coordinates.
inline int xyToIndex(int x, int y, int gridWidth, int gridHeight) {
    if (x < 0 || x >= gridWidth || y < 0 || y >= gridHeight) {
        return -1;
    }
    int rowStart = y * gridWidth;
    if (y % 2 == 0) {
        return rowStart + x;
    } else {
        return rowStart + (gridWidth - 1 - x);
    }
}

// Convert linear LED index to (x, y) grid coordinates for serpentine wiring.
// Sets x = y = -1 for out-of-bounds indices.
inline void indexToXY(int index, int& x, int& y, int gridWidth, int numPixels) {
    if (index < 0 || index >= numPixels) {
        x = -1;
        y = -1;
        return;
    }
    y = index / gridWidth;
    int rowIndex = index - y * gridWidth;
    if (y % 2 == 0) {
        x = rowIndex;
    } else {
        x = gridWidth - 1 - rowIndex;
    }
}

// Euclidean distance between two grid points.
inline float distance2D(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return sqrtf((float)(dx * dx + dy * dy));
}

// Simplified Perlin-like noise using fractional Brownian motion.
// Returns a value in [0, 1].
inline float perlinNoise2D(float x, float y, float time) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;

    for (int i = 0; i < 3; i++) {
        value += sinf(x * frequency + time) * cosf(y * frequency + time) * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return (value + 2.0f) / 4.0f;
}

// Look up a row of a 5x7 bitmap font character.
// Supports: space, G, A, M, E, O, V, R (case-insensitive).
// Returns 0 for unknown characters or out-of-range rows.
inline uint8_t getCharBitmap(char c, int row) {
    static const uint8_t font[][7] = {
        // Space
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        // G
        {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},
        // A
        {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
        // M
        {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11},
        // E
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
        // O
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
        // V
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
        // R
        {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    };

    int index = -1;
    if (c == ' ') index = 0;
    else if (c == 'G' || c == 'g') index = 1;
    else if (c == 'A' || c == 'a') index = 2;
    else if (c == 'M' || c == 'm') index = 3;
    else if (c == 'E' || c == 'e') index = 4;
    else if (c == 'O' || c == 'o') index = 5;
    else if (c == 'V' || c == 'v') index = 6;
    else if (c == 'R' || c == 'r') index = 7;

    if (index >= 0 && row >= 0 && row < 7) {
        return font[index][row];
    }
    return 0;
}

} // namespace MatrixMath

#endif // MATRIX_MATH_H
