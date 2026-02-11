#pragma once

#include "inverted_twinkle_base.h"

class InvertedSplitComplementaryTwinkle : public InvertedTwinkleBase {
public:
    const char* getName() const override { return "Inv. Split-Comp Twinkle"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 150, 210};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 3; }
};
