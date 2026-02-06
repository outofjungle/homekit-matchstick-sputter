#pragma once

#include "../markov_base_layer.h"
#include "../gaussian_blend.h"

// Base class for all harmony-based runner animations
//
// Base layer: All LEDs show channel's hue with per-LED random-walk undulations
//   - Hue: ±ANGLE_WIDTH/2 around channel hue, using Markov chain
//   - Brightness: BASE_BRIGHTNESS to MAX_BRIGHTNESS, using Markov chain
//   - Markov chain has momentum (60% chance to continue current direction)
//
// Runner layer: Groups of LEDs travel from position 0 to end, colored by harmony
//   - Runner count: 1 (at brightness=0) to 4 (at brightness=100)
//   - Length: RUNNER_LENGTH LEDs
//   - Gaussian blending: Bell curve blend between base and runner colors
//
// Derived classes implement getHarmonyOffsets(), getNumHarmonyHues(), getName()
class RunnerAnimationBase : public MarkovBaseLayer
{
public:
    // Tunable parameters (MAX_LEDS, FRAME_MS, ANGLE_WIDTH, BASE_BRIGHTNESS, MAX_BRIGHTNESS inherited from MarkovBaseLayer)
    static constexpr uint8_t RUNNER_LENGTH = 30;     // LEDs per runner
    static constexpr float GAUSSIAN_VARIANCE = 2.5f; // Gaussian blend width (~6-8 pixel blob)
    static constexpr uint8_t MIN_RUNNERS = 1;        // At brightness=100
    static constexpr uint8_t MAX_RUNNERS = 6;        // At brightness=0
    static constexpr uint8_t MAX_RUNNER_SLOTS = 6;   // Per channel

    struct Runner
    {
        int16_t headPos; // Head position on strip (-RUNNER_LENGTH to numLeds-1)
        uint8_t hue;     // FastLED hue (0-255)
        uint8_t sat;     // Saturation (255 for harmony, 0 for white/black)
        uint8_t val;     // Value (255 normally, 0 for black)
        bool active;     // Currently on strip?
    };

    RunnerAnimationBase()
    {
        reset();
    }

    // Set channel hues (called by manager when animation starts or hue changes)
    void setChannelHues(int h1, int h2, int h3, int h4) override
    {
        AnimationBase::setChannelHues(h1, h2, h3, h4);
    }

    // Set channel brightnesses (affects runner count)
    void setChannelBrightnesses(int b1, int b2, int b3, int b4) override
    {
        AnimationBase::setChannelBrightnesses(b1, b2, b3, b4);
    }

    void begin() override
    {
        reset();
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;

        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateBaseLayer();
            updateRunners();
            return true; // Update needed
        }

        return false; // No update needed yet
    }

    void render(CRGB *ch1, CRGB *ch2, CRGB *ch3, CRGB *ch4, uint16_t numLeds) override
    {
        renderChannel(ch1, numLeds, 0);
        renderChannel(ch2, numLeds, 1);
        renderChannel(ch3, numLeds, 2);
        renderChannel(ch4, numLeds, 3);
    }

    void reset() override
    {
        // Precompute Gaussian blend LUT
        gaussianLUT.compute(GAUSSIAN_VARIANCE);

        // Initialize base layer state
        for (int ch = 0; ch < 4; ch++)
        {
            for (int i = 0; i < MAX_LEDS; i++)
            {
                hueOffset[ch][i] = 0;
                hueDir[ch][i] = 0;
                baseBrightness[ch][i] = BASE_BRIGHTNESS;
                brightDir[ch][i] = 0;
            }
            cachedBrightness[ch] = 100; // Default to full brightness

            // Deactivate all runners
            for (int r = 0; r < MAX_RUNNER_SLOTS; r++)
            {
                runners[ch][r].active = false;
                runners[ch][r].headPos = -RUNNER_LENGTH;
            }
            framesSinceSpawn[ch] = 0;
        }
        frameAccumulator = 0;
    }

protected:
    // Runner state
    Runner runners[4][MAX_RUNNER_SLOTS]; // Per channel
    uint16_t framesSinceSpawn[4];        // Per channel, for spawn probability

    // Gaussian blend lookup table
    GaussianBlendLUT<RUNNER_LENGTH> gaussianLUT;

private:
    // Update runners (spawning and movement)
    void updateRunners()
    {
        for (int ch = 0; ch < 4; ch++)
        {
            // Move existing runners
            for (int r = 0; r < MAX_RUNNER_SLOTS; r++)
            {
                if (runners[ch][r].active)
                {
                    runners[ch][r].headPos++;

                    // Deactivate if off the end
                    if (runners[ch][r].headPos >= MAX_LEDS + RUNNER_LENGTH)
                    {
                        runners[ch][r].active = false;
                    }
                }
            }

            // Check if we can spawn
            bool pixel0Clear = true;
            for (int r = 0; r < MAX_RUNNER_SLOTS; r++)
            {
                if (runners[ch][r].active && runners[ch][r].headPos < RUNNER_LENGTH)
                {
                    pixel0Clear = false;
                    break;
                }
            }

            if (pixel0Clear)
            {
                framesSinceSpawn[ch]++;

                // Calculate max runners based on brightness (inverted: low brightness = more runners)
                int maxRunners = MAX_RUNNERS - (cachedBrightness[ch] * (MAX_RUNNERS - MIN_RUNNERS)) / 100;

                // Count active runners
                int activeCount = 0;
                for (int r = 0; r < MAX_RUNNER_SLOTS; r++)
                {
                    if (runners[ch][r].active)
                        activeCount++;
                }

                if (activeCount < maxRunners)
                {
                    // Calculate spawn chance
                    int targetSpawnInterval = (MAX_LEDS + RUNNER_LENGTH) / maxRunners;
                    int spawnChance = (framesSinceSpawn[ch] * 100) / targetSpawnInterval;
                    if (spawnChance > 100)
                        spawnChance = 100;

                    if (random(100) < spawnChance)
                    {
                        // Spawn a new runner
                        for (int r = 0; r < MAX_RUNNER_SLOTS; r++)
                        {
                            if (!runners[ch][r].active)
                            {
                                runners[ch][r].headPos = 0;
                                pickHarmonyColor(ch, runners[ch][r].hue, runners[ch][r].sat, runners[ch][r].val);
                                runners[ch][r].active = true;
                                framesSinceSpawn[ch] = 0;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                // Can't spawn, don't increment counter
                framesSinceSpawn[ch] = 0;
            }
        }
    }

    // Render a single channel
    void renderChannel(CRGB *leds, uint16_t numLeds, uint8_t channelIndex)
    {
        for (int i = 0; i < numLeds; i++)
        {
            // Compute base color
            int hue360 = (channelHue[channelIndex] + hueOffset[channelIndex][i] + 360) % 360;
            uint8_t hue8 = map(hue360, 0, 360, 0, 255);
            CRGB baseColor = CHSV(hue8, 255, baseBrightness[channelIndex][i]);

            // Check if any runner covers this LED
            CRGB finalColor = baseColor;
            bool inRunner = false;

            for (int r = 0; r < MAX_RUNNER_SLOTS; r++)
            {
                if (!runners[channelIndex][r].active)
                    continue;

                int16_t headPos = runners[channelIndex][r].headPos;
                int16_t tailPos = headPos - RUNNER_LENGTH + 1;

                if (i >= tailPos && i <= headPos)
                {
                    // This LED is in this runner
                    CRGB runnerColor = CHSV(
                        runners[channelIndex][r].hue,
                        runners[channelIndex][r].sat,
                        runners[channelIndex][r].val);

                    // Calculate blend amount using Gaussian LUT
                    int posInRunner = i - tailPos;
                    uint8_t blendFactor = gaussianLUT.table[posInRunner];
                    finalColor = blend(baseColor, runnerColor, blendFactor);

                    inRunner = true;
                    break; // Only apply first runner found
                }
            }

            leds[i] = finalColor;
        }
    }
};
