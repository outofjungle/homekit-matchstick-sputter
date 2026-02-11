#pragma once

#include "inverted_twinkle_base.h"

class InvertedComplementaryTwinkle : public InvertedTwinkleBase {
public:
    const char* getName() const override { return "Inv. Complementary Twinkle"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 180};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 2; }
};
