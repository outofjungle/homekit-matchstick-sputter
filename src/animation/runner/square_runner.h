#pragma once

#include "runner_base.h"

// Square runner animation: four colors evenly spaced around the color wheel (90°)
class SquareRunner : public RunnerAnimationBase {
public:
    const char* getName() const override {
        return "Square Runner";
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
