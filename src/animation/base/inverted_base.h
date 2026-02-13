#pragma once

#include "base_only.h"
#include "../sparkle_base_layer.h"

// Inverted Base Animation
// Dark-field sparkle: 20% of LEDs twinkle with sin8 lifecycle humps.
// Each LED has a fixed birth hue and birth saturation (power-law biased toward
// high saturation). Both brightness and saturation follow the same sin8 hump,
// so LEDs fade up from black with their color and fade back to black together.
class InvertedBaseAnimation : public BaseOnlyAnimation, public SparkleBaseLayer
{
public:
    const char *getName() const override
    {
        return "Inverted Base";
    }

    void begin() override
    {
        initSparkleBaseLayer(channelHue);
        frameAccumulator = 0;
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;

        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateSparkleBaseLayer(channelHue);
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
