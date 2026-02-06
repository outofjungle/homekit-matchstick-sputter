#pragma once
#include <cstdint>
#include <algorithm>

// RGB color structure
struct CRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    CRGB() : r(0), g(0), b(0) {}
    CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

    // Named colors
    static const CRGB Black;
    static const CRGB White;
    static const CRGB Red;
    static const CRGB Green;
    static const CRGB Blue;
};

inline const CRGB CRGB::Black(0, 0, 0);
inline const CRGB CRGB::White(255, 255, 255);
inline const CRGB CRGB::Red(255, 0, 0);
inline const CRGB CRGB::Green(0, 255, 0);
inline const CRGB CRGB::Blue(0, 0, 255);

// HSV color structure
struct CHSV {
    uint8_t h;  // Hue (0-255)
    uint8_t s;  // Saturation (0-255)
    uint8_t v;  // Value/Brightness (0-255)

    CHSV() : h(0), s(0), v(0) {}
    CHSV(uint8_t hue, uint8_t sat, uint8_t val) : h(hue), s(sat), v(val) {}

    // Conversion to RGB
    operator CRGB() const {
        // Simplified HSV to RGB conversion
        if (s == 0) {
            // Grayscale
            return CRGB(v, v, v);
        }

        uint8_t region = h / 43;  // 0-5
        uint8_t remainder = (h - (region * 43)) * 6;

        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region) {
            case 0: return CRGB(v, t, p);
            case 1: return CRGB(q, v, p);
            case 2: return CRGB(p, v, t);
            case 3: return CRGB(p, q, v);
            case 4: return CRGB(t, p, v);
            default: return CRGB(v, p, q);
        }
    }
};

// Blend two CRGB colors
inline CRGB blend(const CRGB& a, const CRGB& b, uint8_t amount) {
    uint8_t invAmount = 255 - amount;
    return CRGB(
        (a.r * invAmount + b.r * amount) / 255,
        (a.g * invAmount + b.g * amount) / 255,
        (a.b * invAmount + b.b * amount) / 255
    );
}

// Fill array with solid color
inline void fill_solid(CRGB* leds, uint16_t numLeds, const CRGB& color) {
    for (uint16_t i = 0; i < numLeds; i++) {
        leds[i] = color;
    }
}
