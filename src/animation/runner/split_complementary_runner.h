#pragma once

#include "runner_base.h"

// Split-complementary runner animation: primary + two colors adjacent to complement
class SplitComplementaryRunner : public RunnerAnimationBase {
public:
    const char* getName() const override {
        return "Split-Complementary Runner";
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
