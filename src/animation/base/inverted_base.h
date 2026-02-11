#pragma once

#include "base_only.h"

// Inverted Base Animation
// Dark shimmer: hue and saturation walk identically to the normal base layer,
// but brightness is power-law-biased toward near-black. Most LEDs stay under
// ~5% brightness (13/255); occasional drift up to DARK_MAX_BRIGHTNESS (~20%).
// No bright flares — the strip stays dim at all times.
class InvertedBaseAnimation : public BaseOnlyAnimation
{
public:
    const char *getName() const override
    {
        return "Inverted Base";
    }

    void begin() override
    {
        initInvertedBaseLayer();
        frameAccumulator = 0;
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;

        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateInvertedBaseLayer();
            return true;
        }

        return false;
    }

    void reset() override
    {
        begin();
    }
};
