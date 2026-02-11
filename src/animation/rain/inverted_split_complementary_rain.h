#pragma once

#include "inverted_rain_base.h"

class InvertedSplitComplementaryRain : public InvertedRainBase {
public:
    const char* getName() const override { return "Inv. Split-Comp Rain"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 150, 210};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 3; }
};
