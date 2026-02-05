#pragma once

#include "rain_base.h"

// Triadic rain animation: three evenly spaced colors (0°, 120°, 240°)
class TriadicRain : public RainAnimationBase {
public:
    const char* getName() const override {
        return "Triadic Rain";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 120, 240};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 3;
    }
};
