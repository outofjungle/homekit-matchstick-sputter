#pragma once

#include "harmony_twinkle_base.h"

// Square Twinkle Animation
// 4-color harmony: Square on color wheel (90° apart)
// Example: Red (0°) + Yellow (90°) + Cyan (180°) + Magenta (270°)
class SquareTwinkle : public HarmonyTwinkleBase {
public:
    const char* getName() const override {
        return "Square Twinkle";
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
