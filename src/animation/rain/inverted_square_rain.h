#pragma once

#include "inverted_rain_base.h"

class InvertedSquareRain : public InvertedRainBase {
public:
    const char* getName() const override { return "Inv. Square Rain"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 90, 180, 270};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 4; }
};
