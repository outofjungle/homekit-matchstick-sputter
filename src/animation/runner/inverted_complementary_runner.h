#pragma once

#include "inverted_runner_base.h"

class InvertedComplementaryRunner : public InvertedRunnerBase {
public:
    const char* getName() const override { return "Inv. Complementary Runner"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 180};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 2; }
};
