#pragma once

#include "runner_base.h"

// Triadic runner animation: three colors evenly spaced around the color wheel (120°)
class TriadicRunner : public RunnerAnimationBase {
public:
    const char* getName() const override {
        return "Triadic Runner";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 120, 240};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 3;
    }
};
