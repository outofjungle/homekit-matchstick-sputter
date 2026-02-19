#pragma once

#include "twinkle_base.h"
#include "../sparkle_base_layer.h"

// Base class for inverted-base twinkle animations
// Dark-field sparkle base layer with twinkle overlay
class InvertedTwinkleBase : public TwinkleAnimationBase, public SparkleBaseLayer
{
public:
    void begin() override
    {
        resetBaseLayer();  // initializes all Markov arrays (hue, brightness, saturation dirs)
        initSparkleBaseLayer(channelHue);
        for (int ch = 0; ch < 4; ch++)
        {
            for (int t = 0; t < MAX_TWINKLE_SLOTS; t++)
            {
                twinkles[ch][t].phase = PHASE_NONE;
                twinkles[ch][t].ledIndex = 0;
                twinkles[ch][t].frameCounter = 0;
            }
            rampFrame[ch] = 0;  // intentional: no stagger — all channels ramp together from silence
        }
        frameAccumulator = 0;
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;
        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateSparkleBaseLayer(channelHue);
            updateTwinkles();
            return true;
        }
        return false;
    }

    void reset() override
    {
        begin();
    }

protected:
    CRGB getBaseLedColor(int channelIndex, int ledIndex) const override
    {
        return computeSparkleColor(channelIndex, ledIndex);
    }
};
