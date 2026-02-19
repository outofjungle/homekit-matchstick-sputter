#pragma once

#include "rain_base.h"
#include "../sparkle_base_layer.h"

// Base class for inverted-base rain animations
// Dark-field sparkle base layer with raindrop overlay
class InvertedRainBase : public RainAnimationBase, public SparkleBaseLayer
{
public:
    void begin() override
    {
        resetBaseLayer();
        initSparkleBaseLayer(channelHue);
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
            updateSparkleBaseLayer(channelHue);
            updateRaindrops();
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
