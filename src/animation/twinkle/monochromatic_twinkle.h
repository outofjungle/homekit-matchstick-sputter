#pragma once

#include "twinkle_base.h"

// Monochromatic Twinkle Animation
// 1-color harmony: Primary hue only
// Uses analogous spread (±ANGLE_WIDTH/2 around primary) for variation
class MonochromaticTwinkle : public TwinkleAnimationBase
{
public:
    const char *getName() const override
    {
        return "Monochromatic Twinkle";
    }

protected:
    const int *getHarmonyOffsets() const override
    {
        static const int offsets[] = {0};
        return offsets;
    }

    int getNumHarmonyHues() const override
    {
        return 1;
    }
};
