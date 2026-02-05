#pragma once

#include "rain_base.h"

// Complementary rain animation: primary hue + opposite (180°)
class ComplementaryRain : public RainAnimationBase {
public:
    const char* getName() const override {
        return "Complementary Rain";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 180};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 2;
    }
};
