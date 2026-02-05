#pragma once

#include "rain_base.h"

// Square rain animation: four evenly spaced colors (0°, 90°, 180°, 270°)
class SquareRain : public RainAnimationBase {
public:
    const char* getName() const override {
        return "Square Rain";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 90, 180, 270};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 4;
    }
};
