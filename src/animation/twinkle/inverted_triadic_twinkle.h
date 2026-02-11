#pragma once

#include "inverted_twinkle_base.h"

class InvertedTriadicTwinkle : public InvertedTwinkleBase {
public:
    const char* getName() const override { return "Inv. Triadic Twinkle"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 120, 240};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 3; }
};
