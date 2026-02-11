#pragma once

#include "inverted_runner_base.h"

class InvertedSquareRunner : public InvertedRunnerBase {
public:
    const char* getName() const override { return "Inv. Square Runner"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 90, 180, 270};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 4; }
};
