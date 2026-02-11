#pragma once

#include "inverted_rain_base.h"

class InvertedMonochromaticRain : public InvertedRainBase {
public:
    const char* getName() const override { return "Inv. Monochromatic Rain"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 1; }
};
