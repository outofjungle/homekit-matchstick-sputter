#pragma once

#include "runner_base.h"

// Base class for inverted-base runner animations
// Dark-shimmer base layer (power-law-biased toward black) with runner overlay
class InvertedRunnerBase : public RunnerAnimationBase
{
public:
    // NOTE: This begin() intentionally duplicates RunnerAnimationBase::reset() logic, but calls
    // initInvertedBaseLayer() instead of resetBaseLayer(). This is accepted duplication — the
    // inheritance design requires the swap to apply dark-shimmer brightness initialization.
    void begin() override
    {
        gaussianLUT.compute(GAUSSIAN_VARIANCE);
        initInvertedBaseLayer();
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
            updateInvertedBaseLayer();
            updateRunners();
            return true;
        }
        return false;
    }

    void reset() override
    {
        begin();
    }
};
