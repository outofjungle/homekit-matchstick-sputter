#pragma once

#include "inverted_rain_base.h"

class InvertedTriadicRain : public InvertedRainBase {
public:
    const char* getName() const override { return "Inv. Triadic Rain"; }
protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0, 120, 240};
        return offsets;
    }
    int getNumHarmonyHues() const override { return 3; }
};
