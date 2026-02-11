#pragma once

#include "rain_base.h"

// Base class for inverted-base rain animations
// Dark-shimmer base layer (power-law-biased toward black) with raindrop overlay
class InvertedRainBase : public RainAnimationBase
{
public:
    // NOTE: This begin() intentionally duplicates RainAnimationBase::reset() logic, but calls
    // initInvertedBaseLayer() instead of resetBaseLayer(). This is accepted duplication — the
    // inheritance design requires the swap to apply dark-shimmer brightness initialization.
    void begin() override
    {
        initInvertedBaseLayer();
        for (int ch = 0; ch < 4; ch++)
        {
            for (int r = 0; r < MAX_RAINDROP_SLOTS; r++)
            {
                raindrops[ch][r].active = false;
                raindrops[ch][r].currentFrame = 0;
                raindrops[ch][r].centerPos = 0;
            }
            framesSinceSpawn[ch] = 0;
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
            updateRaindrops();
            return true;
        }
        return false;
    }

    void reset() override
    {
        begin();
    }
};
