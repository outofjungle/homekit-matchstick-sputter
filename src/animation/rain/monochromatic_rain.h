#pragma once

#include "rain_base.h"

// Monochromatic rain animation: primary hue and white raindrops
class MonochromaticRain : public RainAnimationBase {
public:
    const char* getName() const override {
        return "Monochromatic Rain";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 1;
    }

    // Override to use primary hue/white instead of harmony hues
    void pickRaindropColor(int channelIndex, uint8_t& h, uint8_t& s, uint8_t& v) override {
        // 50/50 chance of primary hue or white
        if (random(2) == 0) {
            // Primary hue (use base class implementation)
            RainAnimationBase::pickRaindropColor(channelIndex, h, s, v);
        } else {
            // White
            h = 0;
            s = 0;
            v = 255;
        }
    }
};
