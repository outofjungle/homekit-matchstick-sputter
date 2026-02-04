#pragma once

#include "harmony_twinkle_base.h"

// Complementary Twinkle Animation
// 2-color harmony: Primary + Opposite (180° apart)
// Example: Red (0°) + Cyan (180°)
class ComplementaryTwinkle : public HarmonyTwinkleBase {
public:
    const char* getName() const override {
        return "Complementary Twinkle";
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
