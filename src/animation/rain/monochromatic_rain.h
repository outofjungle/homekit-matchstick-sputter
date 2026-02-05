#pragma once

#include "rain_base.h"

// Monochromatic rain animation: primary hue (white via PRIMARY_HUE_SAT=0)
class MonochromaticRain : public RainAnimationBase {
public:
    const char* getName() const override {
        return "Monochromatic Rain";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 1;
    }
};
