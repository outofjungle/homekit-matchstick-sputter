#pragma once

#include "inverted_runner_base.h"

class InvertedTriadicRunner : public InvertedRunnerBase {
public:
    const char* getName() const override { return "Inv. Triadic Runner"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 120, 240};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 3; }
};
