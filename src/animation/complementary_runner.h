#pragma once

#include "runner_base.h"

// Complementary runner animation: uses primary hue and its opposite (180°)
class ComplementaryRunner : public RunnerAnimationBase {
public:
    const char* getName() const override {
        return "Complementary Runner";
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
