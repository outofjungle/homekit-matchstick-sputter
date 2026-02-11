#pragma once

#include "twinkle_base.h"

// Base class for inverted-base twinkle animations
// Dark-shimmer base layer (power-law-biased toward black) with twinkle overlay
class InvertedTwinkleBase : public TwinkleAnimationBase
{
public:
    void begin() override
    {
        initInvertedBaseLayer();
        for (int ch = 0; ch < 4; ch++)
        {
            for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
            {
                twinkles[ch][t].phase = PHASE_NONE;
                twinkles[ch][t].ledIndex = 0;
                twinkles[ch][t].frameCounter = 0;
            }
            rampFrame[ch] = 0;
        }
        frameAccumulator = 0;
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;
        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateInvertedBaseLayer();
            updateTwinkles();
            return true;
        }
        return false;
    }

    void reset() override
    {
        begin();
    }
};
