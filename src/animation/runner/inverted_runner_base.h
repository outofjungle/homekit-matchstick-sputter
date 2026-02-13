#pragma once

#include "runner_base.h"
#include "../sparkle_base_layer.h"

// Base class for inverted-base runner animations
// Dark-field sparkle base layer with runner overlay
class InvertedRunnerBase : public RunnerAnimationBase, public SparkleBaseLayer
{
public:
    void begin() override
    {
        gaussianLUT.compute(GAUSSIAN_VARIANCE);
        initSparkleBaseLayer(channelHue);
        for (int ch = 0; ch < 4; ch++)
        {
            for (int r = 0; r < MAX_RUNNER_SLOTS; r++)
            {
                runners[ch][r].active = false;
                runners[ch][r].headPos = -RUNNER_LENGTH;
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
            updateRunners();
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
