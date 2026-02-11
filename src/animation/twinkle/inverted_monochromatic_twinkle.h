#pragma once

#include "inverted_twinkle_base.h"

class InvertedMonochromaticTwinkle : public InvertedTwinkleBase {
public:
    const char* getName() const override { return "Inv. Monochromatic Twinkle"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 1; }
};
