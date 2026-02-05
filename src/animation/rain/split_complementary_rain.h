#pragma once

#include "rain_base.h"

// Split-complementary rain animation: primary hue + two adjacent to opposite
class SplitComplementaryRain : public RainAnimationBase {
public:
    const char* getName() const override {
        return "Split-Complementary Rain";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 150, 210};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 3;
    }
};
