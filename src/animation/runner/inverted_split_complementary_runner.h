#pragma once

#include "inverted_runner_base.h"

class InvertedSplitComplementaryRunner : public InvertedRunnerBase {
public:
    const char* getName() const override { return "Inv. Split-Comp Runner"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 150, 210};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 3; }
};
