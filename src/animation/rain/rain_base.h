#pragma once

#include "../animation_base.h"
#include <math.h>

// Base class for all harmony-based rain animations
//
// Base layer: All LEDs show channel's hue with per-LED random-walk undulations
//   - Hue: ±ANGLE_WIDTH/2 around channel hue, using Markov chain
//   - Brightness: BASE_BRIGHTNESS to MAX_BRIGHTNESS, using Markov chain
//   - Markov chain has momentum (60% chance to continue current direction)
//
// Raindrop layer: Random stationary "raindrops" fade in/out using time-varying Gaussian blend
//   - Raindrop count: 1 (at brightness=100) to 6 (at brightness=0)
//   - Length: RAINDROP_LENGTH LEDs (centered at spawn position)
//   - Lifecycle: RAINDROP_MAX_FRAMES frames
//   - Gaussian variance: 0.1 (frame 0) → 10.0 (frame MAX) for fade effect
//   - Spawn: Random non-colliding positions
//
// Derived classes implement getHarmonyOffsets(), getNumHarmonyHues(), getName()
class RainAnimationBase : public AnimationBase
{
public:
    // Tunable parameters
    static constexpr uint16_t MAX_LEDS = 200;
    static constexpr unsigned long FRAME_MS = 50;    // 20fps
    static constexpr int ANGLE_WIDTH = 10;           // ±5° hue spread
    static constexpr uint8_t BASE_BRIGHTNESS = 40;   // Min breathing brightness
    static constexpr uint8_t MAX_BRIGHTNESS = 220;   // Max breathing brightness
    static constexpr uint8_t RAINDROP_LENGTH = 11;   // LEDs per raindrop (must be odd)
    static constexpr uint8_t RAINDROP_MAX_FRAMES = 30; // 1.5s lifecycle
    static constexpr float MIN_GAUSSIAN_VARIANCE = 0.1f; // Frame 0 (concentrated)
    static constexpr float MAX_GAUSSIAN_VARIANCE = 10.0f; // Frame MAX (diffuse)
    static constexpr uint8_t MIN_RAINDROPS = 6;      // At brightness=100
    static constexpr uint8_t MAX_RAINDROPS = 18;     // At brightness=0
    static constexpr uint8_t MAX_RAINDROP_SLOTS = 18; // Per channel
    static constexpr uint8_t MAX_SPAWN_ATTEMPTS = 10; // Collision retry limit

    struct Raindrop
    {
        int16_t centerPos;  // Center position on strip
        uint8_t currentFrame; // Lifecycle frame (0 to MAX_FRAMES)
        uint8_t hue;        // FastLED hue (0-255)
        uint8_t sat;        // Saturation (255 for harmony, 0 for white/black)
        uint8_t val;        // Value (255 normally, 0 for black)
        bool active;        // Currently alive?
    };

    RainAnimationBase()
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

    // Set channel brightnesses (affects raindrop count)
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
            updateRaindrops();
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

            // Deactivate all raindrops
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

protected:
    // Per-LED base state (4 channels × 200 LEDs)
    int8_t hueOffset[4][MAX_LEDS];       // Current offset from channel hue (-ANGLE_WIDTH/2 to +ANGLE_WIDTH/2)
    int8_t hueDir[4][MAX_LEDS];          // Last hue move direction: -1, 0, +1
    uint8_t baseBrightness[4][MAX_LEDS]; // Current base brightness
    int8_t brightDir[4][MAX_LEDS];       // Last brightness move direction: -1, 0, +1

    // Raindrop state
    Raindrop raindrops[4][MAX_RAINDROP_SLOTS]; // Per channel
    uint16_t framesSinceSpawn[4];              // Per channel, for spawn probability

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

    // Override in derived classes if needed (e.g., MonochromaticRain)
    virtual void pickRaindropColor(int channelIndex, uint8_t &h, uint8_t &s, uint8_t &v)
    {
        const int *offsets = getHarmonyOffsets();
        int idx = random(getNumHarmonyHues());
        int hue360 = (channelHue[channelIndex] + offsets[idx] + 360) % 360;
        int spread = generateSpread();
        hue360 = (hue360 + spread + 360) % 360;
        h = map(hue360, 0, 360, 0, 255);
        s = (offsets[idx] == 0) ? PRIMARY_HUE_SAT : 255;  // Desaturate primary hue
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
                    nextHueDir = markovTransition(-1);
                }
                else if (hueOffset[ch][i] <= -ANGLE_WIDTH / 2 && nextHueDir < 0)
                {
                    nextHueDir = markovTransition(1);
                }

                hueDir[ch][i] = nextHueDir;
                hueOffset[ch][i] += nextHueDir;
                hueOffset[ch][i] = constrain(hueOffset[ch][i], -ANGLE_WIDTH / 2, ANGLE_WIDTH / 2);

                // Brightness random walk (biased towards brighter)
                int8_t nextBrightDir = markovTransitionBrightnessBiased(brightDir[ch][i]);

                // Limit bouncing at MAX with optional knock-to-zero effect
                if (baseBrightness[ch][i] >= MAX_BRIGHTNESS && nextBrightDir > 0)
                {
                    if (random(100) < BRIGHTNESS_KNOCK_ZERO_PCT) {
                        baseBrightness[ch][i] = 0;
                        brightDir[ch][i] = 0;
                        continue;  // Skip normal step+constrain
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

    // Check if position collides with any active raindrop
    bool checkCollision(int channelIndex, int16_t pos)
    {
        for (int r = 0; r < MAX_RAINDROP_SLOTS; r++)
        {
            if (raindrops[channelIndex][r].active)
            {
                int16_t distance = abs(pos - raindrops[channelIndex][r].centerPos);
                if (distance < RAINDROP_LENGTH)
                {
                    return true; // Collision detected
                }
            }
        }
        return false; // No collision
    }

    // Find a random non-colliding spawn position
    bool findSpawnPosition(int channelIndex, int16_t &outPos)
    {
        for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++)
        {
            int16_t candidatePos = random(0, MAX_LEDS);
            if (!checkCollision(channelIndex, candidatePos))
            {
                outPos = candidatePos;
                return true; // Found valid position
            }
        }
        return false; // Failed to find position after max attempts
    }

    // Update raindrops (aging and spawning)
    void updateRaindrops()
    {
        for (int ch = 0; ch < 4; ch++)
        {
            // Age existing raindrops
            for (int r = 0; r < MAX_RAINDROP_SLOTS; r++)
            {
                if (raindrops[ch][r].active)
                {
                    raindrops[ch][r].currentFrame++;

                    // Deactivate if lifecycle complete
                    if (raindrops[ch][r].currentFrame >= RAINDROP_MAX_FRAMES)
                    {
                        raindrops[ch][r].active = false;
                    }
                }
            }

            framesSinceSpawn[ch]++;

            // Calculate max raindrops based on brightness (inverted: low brightness = more raindrops)
            int maxRaindrops = MAX_RAINDROPS - (cachedBrightness[ch] * (MAX_RAINDROPS - MIN_RAINDROPS)) / 100;

            // Count active raindrops
            int activeCount = 0;
            for (int r = 0; r < MAX_RAINDROP_SLOTS; r++)
            {
                if (raindrops[ch][r].active)
                    activeCount++;
            }

            if (activeCount < maxRaindrops)
            {
                // Calculate spawn chance
                int targetSpawnInterval = (MAX_LEDS + RAINDROP_LENGTH) / maxRaindrops;
                int spawnChance = (framesSinceSpawn[ch] * 100) / targetSpawnInterval;
                if (spawnChance > 100)
                    spawnChance = 100;

                if (random(100) < spawnChance)
                {
                    // Try to spawn a new raindrop
                    int16_t spawnPos;
                    if (findSpawnPosition(ch, spawnPos))
                    {
                        for (int r = 0; r < MAX_RAINDROP_SLOTS; r++)
                        {
                            if (!raindrops[ch][r].active)
                            {
                                raindrops[ch][r].centerPos = spawnPos;
                                raindrops[ch][r].currentFrame = 0;
                                pickRaindropColor(ch, raindrops[ch][r].hue, raindrops[ch][r].sat, raindrops[ch][r].val);
                                raindrops[ch][r].active = true;
                                framesSinceSpawn[ch] = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // Compute Gaussian blend factor for a raindrop at a given position
    // Uses time-varying variance based on raindrop's current frame
    uint8_t computeRaindropBlend(const Raindrop &drop, int16_t ledPos)
    {
        // Calculate distance from raindrop center
        float x = (float)(ledPos - drop.centerPos);

        // Calculate time-varying variance (0.1 → 10.0 over lifecycle)
        float frameProgress = drop.currentFrame / (float)RAINDROP_MAX_FRAMES;
        float variance = MIN_GAUSSIAN_VARIANCE + frameProgress * (MAX_GAUSSIAN_VARIANCE - MIN_GAUSSIAN_VARIANCE);

        // Gaussian spatial factor
        float spatialFactor = expf(-(x * x) / (2.0f * variance));

        // Temporal decay factor (fade over lifetime)
        float temporalFactor = 1.0f - frameProgress;

        // Combined blend factor
        float blendFactor = spatialFactor * temporalFactor;

        return (uint8_t)constrain((int)(blendFactor * 255.0f + 0.5f), 0, 255);
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

            // Check if any raindrop covers this LED
            CRGB finalColor = baseColor;

            for (int r = 0; r < MAX_RAINDROP_SLOTS; r++)
            {
                if (!raindrops[channelIndex][r].active)
                    continue;

                const Raindrop &drop = raindrops[channelIndex][r];
                int16_t halfLen = RAINDROP_LENGTH / 2;
                int16_t minPos = drop.centerPos - halfLen;
                int16_t maxPos = drop.centerPos + halfLen;

                // Check if LED is within raindrop range
                if (i >= minPos && i <= maxPos)
                {
                    // This LED is in this raindrop
                    CRGB raindropColor = CHSV(drop.hue, drop.sat, drop.val);

                    // Calculate blend amount using time-varying Gaussian
                    uint8_t blendFactor = computeRaindropBlend(drop, i);
                    finalColor = blend(baseColor, raindropColor, blendFactor);

                    break; // Only apply first raindrop found
                }
            }

            leds[i] = finalColor;
        }
    }
};
