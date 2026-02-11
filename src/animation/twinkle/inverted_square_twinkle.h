#pragma once

#include "inverted_twinkle_base.h"

class InvertedSquareTwinkle : public InvertedTwinkleBase {
public:
    const char* getName() const override { return "Inv. Square Twinkle"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 90, 180, 270};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 4; }
};
