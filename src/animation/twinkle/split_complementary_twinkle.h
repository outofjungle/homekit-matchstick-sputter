#pragma once

#include "twinkle_base.h"

// Split-Complementary Twinkle Animation
// 3-color harmony: Primary + Two adjacent to opposite (150° and 210°)
// Example: Red (0°) + Yellow-Green (150°) + Blue-Violet (210°)
class SplitComplementaryTwinkle : public TwinkleAnimationBase
{
public:
    const char *getName() const override
    {
        return "Split-Complementary Twinkle";
    }

protected:
    const int *getHarmonyOffsets() const override
    {
        static const int offsets[] = {0, 150, 210};
        return offsets;
    }

    int getNumHarmonyHues() const override
    {
        return 3;
    }
};
