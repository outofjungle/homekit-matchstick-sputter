#pragma once

#include "../animation_base.h"
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
class RunnerAnimationBase : public AnimationBase
{
public:
    // Tunable parameters
    static constexpr uint16_t MAX_LEDS = 200;
    static constexpr unsigned long FRAME_MS = 50;    // 20fps
    static constexpr int ANGLE_WIDTH = 10;           // ±5° hue spread
    static constexpr uint8_t BASE_BRIGHTNESS = 40;   // Min breathing brightness
    static constexpr uint8_t MAX_BRIGHTNESS = 220;   // Max breathing brightness
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
    void setChannelHues(int h1, int h2, int h3, int h4)
    {
        channelHue[0] = h1;
        channelHue[1] = h2;
        channelHue[2] = h3;
        channelHue[3] = h4;
    }

    // Set channel brightnesses (affects runner count)
    void setChannelBrightnesses(int b1, int b2, int b3, int b4)
    {
        cachedBrightness[0] = b1;
        cachedBrightness[1] = b2;
        cachedBrightness[2] = b3;
        cachedBrightness[3] = b4;
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
    // Per-LED base state (4 channels × 200 LEDs)
    int8_t hueOffset[4][MAX_LEDS];       // Current offset from channel hue (-ANGLE_WIDTH/2 to +ANGLE_WIDTH/2)
    int8_t hueDir[4][MAX_LEDS];          // Last hue move direction: -1, 0, +1
    uint8_t baseBrightness[4][MAX_LEDS]; // Current base brightness
    int8_t brightDir[4][MAX_LEDS];       // Last brightness move direction: -1, 0, +1

    // Runner state
    Runner runners[4][MAX_RUNNER_SLOTS]; // Per channel
    uint16_t framesSinceSpawn[4];        // Per channel, for spawn probability

    // Gaussian blend lookup table
    GaussianBlendLUT<RUNNER_LENGTH> gaussianLUT;

    // Cached channel state
    int channelHue[4];
    int cachedBrightness[4];

    // Frame timing
    unsigned long frameAccumulator = 0;

    // Derived classes implement these to define the harmony
    virtual const int *getHarmonyOffsets() const = 0; // Hue offsets from primary (0°, ...)
    virtual int getNumHarmonyHues() const = 0;        // Number of hues in harmony

    // Generate analogous spread offset using normal distribution approximation
    int generateSpread()
    {
        int sum = 0;
        for (int i = 0; i < 6; i++)
        {
            sum += random(0, ANGLE_WIDTH + 1);
        }
        return (sum / 6) - (ANGLE_WIDTH / 2); // Centered at 0
    }

    // Override in derived classes if needed (e.g., MonochromaticRunner)
    virtual void pickRunnerColor(int channelIndex, uint8_t &h, uint8_t &s, uint8_t &v)
    {
        const int *offsets = getHarmonyOffsets();
        int idx = random(getNumHarmonyHues());
        int hue360 = (channelHue[channelIndex] + offsets[idx] + 360) % 360;
        int spread = generateSpread();
        hue360 = (hue360 + spread + 360) % 360;
        h = map(hue360, 0, 360, 0, 255);
        s = (offsets[idx] == 0) ? PRIMARY_HUE_SAT : 255; // Desaturate primary hue
        v = 255;
    }

private:
    // Markov chain transition: returns -1, 0, or +1
    // Momentum: 60% chance to continue current direction
    int markovTransition(int8_t currentDir)
    {
        int roll = random(100);

        if (currentDir == 0)
        {
            // No prior direction: equal probability
            if (roll < 33)
                return -1;
            if (roll < 67)
                return 0;
            return 1;
        }
        else if (currentDir > 0)
        {
            // Moving positive: 60% stay positive, 20% stay, 20% reverse
            if (roll < 60)
                return 1;
            if (roll < 80)
                return 0;
            return -1;
        }
        else
        {
            // Moving negative: 60% stay negative, 20% stay, 20% reverse
            if (roll < 60)
                return -1;
            if (roll < 80)
                return 0;
            return 1;
        }
    }

    // Update base layer undulations
    void updateBaseLayer()
    {
        for (int ch = 0; ch < 4; ch++)
        {
            for (int i = 0; i < MAX_LEDS; i++)
            {
                // Hue random walk
                int8_t nextHueDir = markovTransition(hueDir[ch][i]);

                // Limit bouncing: flip bias if at limits
                if (hueOffset[ch][i] >= ANGLE_WIDTH / 2 && nextHueDir > 0)
                {
                    nextHueDir = markovTransition(-1); // Treat as if moving negative
                }
                else if (hueOffset[ch][i] <= -ANGLE_WIDTH / 2 && nextHueDir < 0)
                {
                    nextHueDir = markovTransition(1); // Treat as if moving positive
                }

                hueDir[ch][i] = nextHueDir;
                hueOffset[ch][i] += nextHueDir;
                hueOffset[ch][i] = constrain(hueOffset[ch][i], -ANGLE_WIDTH / 2, ANGLE_WIDTH / 2);

                // Brightness random walk (biased towards brighter)
                int8_t nextBrightDir = markovTransitionBrightnessBiased(brightDir[ch][i]);

                // Limit bouncing at MAX with optional knock-to-zero effect
                if (baseBrightness[ch][i] >= MAX_BRIGHTNESS && nextBrightDir > 0)
                {
                    if (random(100) < BRIGHTNESS_KNOCK_ZERO_PCT)
                    {
                        baseBrightness[ch][i] = 0;
                        brightDir[ch][i] = 0;
                        continue; // Skip normal step+constrain
                    }
                    nextBrightDir = markovTransitionBrightnessBiased(-1);
                }
                else if (baseBrightness[ch][i] <= BASE_BRIGHTNESS && nextBrightDir < 0)
                {
                    nextBrightDir = markovTransitionBrightnessBiased(1);
                }

                brightDir[ch][i] = nextBrightDir;
                baseBrightness[ch][i] += nextBrightDir * 2; // Step by 2
                baseBrightness[ch][i] = constrain(baseBrightness[ch][i], BASE_BRIGHTNESS, MAX_BRIGHTNESS);
            }
        }
    }

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
                                pickRunnerColor(ch, runners[ch][r].hue, runners[ch][r].sat, runners[ch][r].val);
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
