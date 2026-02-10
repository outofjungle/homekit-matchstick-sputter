#pragma once

#include "twinkle_base.h"

// Triadic Twinkle Animation
// 3-color harmony: Evenly spaced around color wheel (120° apart)
// Example: Red (0°) + Green (120°) + Blue (240°)
class TriadicTwinkle : public TwinkleAnimationBase
{
public:
    const char *getName() const override
    {
        return "Triadic Twinkle";
    }

protected:
    const int *getHarmonyOffsets() const override
    {
        static const int offsets[] = {0, 120, 240};
        return offsets;
    }

    int getNumHarmonyHues() const override
    {
        return 3;
    }
};
