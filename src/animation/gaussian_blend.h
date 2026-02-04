#pragma once

#include <math.h>
#include <Arduino.h>

// Gaussian Blend Lookup Table
// Provides precomputed Gaussian curve values for smooth blending
//
// Usage:
//   GaussianBlendLUT<30> lut;
//   lut.compute(2.5f);  // variance parameter
//   uint8_t blend = lut.table[i];  // 0-255 blend factor
//
// The table is centered, so table[LENGTH/2] is the peak (255)
// and values decay toward 0 at the edges.
template <uint8_t LENGTH>
struct GaussianBlendLUT {
    uint8_t table[LENGTH];

    // Compute Gaussian curve with given variance
    // variance: Controls the width of the curve (higher = wider blob)
    //   - 2.5 gives ~6-8 pixel visible width at center
    //   - 5.0 gives ~12-14 pixel visible width at center
    void compute(float variance) {
        float halfLen = LENGTH / 2.0f;
        for (uint8_t i = 0; i < LENGTH; i++) {
            float x = (i - halfLen);
            float g = expf(-(x * x) / (2.0f * variance));
            int val = (int)(g * 255.0f + 0.5f);
            table[i] = (uint8_t)constrain(val, 0, 255);
        }
    }
};
