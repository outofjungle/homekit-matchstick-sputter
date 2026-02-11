#pragma once

#include "inverted_rain_base.h"

class InvertedComplementaryRain : public InvertedRainBase {
public:
    const char* getName() const override { return "Inv. Complementary Rain"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 180};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 2; }
};
