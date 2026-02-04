#pragma once

#include "runner_base.h"

// Monochromatic runner animation: primary hue and white runners
class MonochromaticRunner : public RunnerAnimationBase {
public:
    const char* getName() const override {
        return "Monochromatic Runner";
    }

protected:
    const int* getHarmonyOffsets() const override {
        static const int offsets[] = {0};
        return offsets;
    }

    int getNumHarmonyHues() const override {
        return 1;
    }

    // Override to use primary hue/white instead of hues
    void pickRunnerColor(int channelIndex, uint8_t& h, uint8_t& s, uint8_t& v) override {
        // 50/50 chance of primary hue or white
        if (random(2) == 0) {
            // Primary hue (use base class implementation)
            RunnerAnimationBase::pickRunnerColor(channelIndex, h, s, v);
        } else {
            // White
            h = 0;
            s = 0;
            v = 255;
        }
    }
};
