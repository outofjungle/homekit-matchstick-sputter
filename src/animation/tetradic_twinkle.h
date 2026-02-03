#pragma once

#include "harmony_twinkle_base.h"

// Tetradic (Rectangle) Twinkle Animation
// 4-color harmony: Rectangle on color wheel
// Example: Red (0°) + Yellow (60°) + Cyan (180°) + Blue (240°)
class TetradicTwinkle : public HarmonyTwinkleBase {
public:
    const char* getName() const override {
        return "Tetradic Twinkle";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 60, 180, 240};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 4;
    }
};
