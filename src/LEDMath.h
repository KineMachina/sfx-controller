#ifndef LED_MATH_H
#define LED_MATH_H

#include <stdint.h>

namespace LEDMath {

// Pack RGB components into a 32-bit color value.
inline uint32_t packColor(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// Extract red component from packed color.
inline uint8_t unpackR(uint32_t color) { return (color >> 16) & 0xFF; }

// Extract green component from packed color.
inline uint8_t unpackG(uint32_t color) { return (color >> 8) & 0xFF; }

// Extract blue component from packed color.
inline uint8_t unpackB(uint32_t color) { return color & 0xFF; }

// HSV-like color wheel: maps 0-255 to a smooth RGB cycle.
// At each position, exactly two channels are active and sum to 255.
inline uint32_t colorWheel(uint8_t wheelPos) {
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return packColor(255 - wheelPos * 3, 0, wheelPos * 3);
    } else if (wheelPos < 170) {
        wheelPos -= 85;
        return packColor(0, wheelPos * 3, 255 - wheelPos * 3);
    } else {
        wheelPos -= 170;
        return packColor(wheelPos * 3, 255 - wheelPos * 3, 0);
    }
}

// Linearly interpolate between two colors. Ratio is clamped to [0, 1].
inline uint32_t blendColor(uint32_t color1, uint32_t color2, float ratio) {
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    uint8_t r = (uint8_t)(unpackR(color1) * (1.0f - ratio) + unpackR(color2) * ratio);
    uint8_t g = (uint8_t)(unpackG(color1) * (1.0f - ratio) + unpackG(color2) * ratio);
    uint8_t b = (uint8_t)(unpackB(color1) * (1.0f - ratio) + unpackB(color2) * ratio);

    return packColor(r, g, b);
}

// Fade a single channel value: (value * fadeValue) >> 8.
// fadeValue=256 preserves full brightness, 0 fades to black.
inline uint8_t fadeChannel(uint8_t value, int fadeValue) {
    return (uint8_t)((value * fadeValue) >> 8);
}

} // namespace LEDMath

#endif // LED_MATH_H
