#pragma once

#include "inverted_runner_base.h"

class InvertedMonochromaticRunner : public InvertedRunnerBase {
public:
    const char* getName() const override { return "Inv. Monochromatic Runner"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 1; }
};
