#pragma once

#include "runner_base.h"

// Monochromatic runner animation: primary hue (white via PRIMARY_HUE_SAT=0)
class MonochromaticRunner : public RunnerAnimationBase {
public:
    const char* getName() const override {
        return "Monochromatic Runner";
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
