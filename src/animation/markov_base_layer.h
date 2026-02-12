#pragma once

#include "animation_base.h"

// Intermediate base class for animations that use Markov chain base layer
//
// Base layer: All LEDs show channel's hue with per-LED random-walk undulations
//   - Hue: ±ANGLE_WIDTH/2 around channel hue, using Markov chain
//   - Brightness: BASE_BRIGHTNESS to MAX_BRIGHTNESS, using Markov chain
//   - Markov chain has momentum (60% chance to continue current direction)
//
// Derived classes (Runner, Rain) add overlay effects on top of this base layer
// and must implement getHarmonyOffsets() and getNumHarmonyHues()
class MarkovBaseLayer : public AnimationBase
{
public:
    // Tunable parameters for base layer
    static constexpr uint8_t BASE_BRIGHTNESS = 40;   // Min breathing brightness
    static constexpr uint8_t MAX_BRIGHTNESS = 220;   // Max breathing brightness

    // Minimum saturation for base layer (0-255, 128 = 50%)
    static constexpr uint8_t MIN_SATURATION = 128;

    // Power law distribution parameter for saturation
    // Target: sat = 255 * (1 - u^α) where u ~ Uniform(0,1)
    // Higher α = stronger skew toward high saturation
    static constexpr float POWER_LAW_ALPHA = 4.0f;

    // Pre-computed bias lookup table for saturation random walk
    // Positive = bias toward increasing saturation
    // Based on power law PDF: ~68% at high saturation (200-255), ~7% very low (0-63)
    static constexpr int8_t SAT_BIAS_TABLE[256] = {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
          1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,
          2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,
          4,   4,   4,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,
          5,   5,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   7,   7,   7,
          7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8,   8,   8,   8,   8,
          8,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,  10,  10,  10,  10,  10,
         10,  10,  10,  10,  10,  11,  11,  11,  11,  11,  11,  11,  11,  11,  12,  12,
         12,  12,  12,  12,  12,  12,  12,  12,  13,  13,  13,  13,  13,  13,  13,  13,
         13,  14,  14,  14,  14,  14,  14,  14,  14,  14,  15,  15,  15,  15,  15,  15,
         15,  15,  15,  16,  16,  16,  16,  16,  16,  16,  16,  16,  17,  17,  17,  17,
         17,  17,  17,  17,  18,  18,  18,  18,  18,  18,  18,  18,  18,  19,  19,  19,
         19,  19,  19,  19,  19,  20,  20,  20,  20,  20,  20,  20,  21,  21,  21,  21,
         21,  21,  21,  21,  22,  22,  22,  22,  22,  22,  22,  23,  23,  23,  23,  23,
         23,  23,  24,  24,  24,  24,  24,  24,  25,  25,  25,  25,  25,  25,  26,  26,
         26,  26,  26,  26,  27,  27,  27,  27,  27,  28,  28,  28,  28,  29,  29,  30,
    };

protected:
    // Per-LED base state (4 channels × MAX_LEDS)
    // RAM: 6 arrays × 4ch × 200 LEDs × 1 byte = ~4,800 bytes per animation instance
    int8_t hueOffset[4][MAX_LEDS];       // Current offset from channel hue (-ANGLE_WIDTH/2 to +ANGLE_WIDTH/2)
    int8_t hueDir[4][MAX_LEDS];          // Last hue move direction: -1, 0, +1
    uint8_t baseBrightness[4][MAX_LEDS]; // Current base brightness
    int8_t brightDir[4][MAX_LEDS];       // Last brightness move direction: -1, 0, +1
    uint8_t baseSaturation[4][MAX_LEDS]; // Current saturation (0-255)
    int8_t satDir[4][MAX_LEDS];          // Last saturation move direction: -1, 0, +1

    // Compute the base layer CRGB color for a single LED
    // Shared helper used by all renderChannel implementations
    CRGB computeBaseColor(int channelIndex, int ledIndex) const {
        int hue360 = (channelHue[channelIndex] + hueOffset[channelIndex][ledIndex] + 360) % 360;
        uint8_t hue8 = map(hue360, 0, 360, 0, 255);
        return CHSV(hue8, baseSaturation[channelIndex][ledIndex], baseBrightness[channelIndex][ledIndex]);
    }

    // Initialize all base layer per-LED state (call from derived class reset())
    void resetBaseLayer()
    {
        for (int ch = 0; ch < 4; ch++)
        {
            for (int i = 0; i < MAX_LEDS; i++)
            {
                hueOffset[ch][i] = random(-ANGLE_WIDTH / 2, ANGLE_WIDTH / 2 + 1);
                hueDir[ch][i] = random(3) - 1;
                baseBrightness[ch][i] = random(BASE_BRIGHTNESS, MAX_BRIGHTNESS + 1);
                brightDir[ch][i] = random(3) - 1;

                // Sample saturation from power law distribution, clamped to [MIN_SATURATION, 255]
                // sat = 255 * (1 - u^α), α=4 skews heavily toward high saturation
                float u = random(1000) / 1000.0f;
                float power_sample = powf(u, POWER_LAW_ALPHA);
                baseSaturation[ch][i] = constrain((int)((1.0f - power_sample) * 255), MIN_SATURATION, 255);
                satDir[ch][i] = random(3) - 1;
            }
            cachedBrightness[ch] = 100;
        }
    }

    // Derived classes implement these to define the harmony
    virtual const int *getHarmonyOffsets() const = 0; // Hue offsets from primary (0°, ...)
    virtual int getNumHarmonyHues() const = 0;        // Number of hues in harmony

    // Pick a harmony color for overlay effects (runners, raindrops, etc.)
    // Override in derived classes if needed (e.g., MonochromaticRunner/Rain)
    virtual void pickHarmonyColor(int channelIndex, uint8_t &h, uint8_t &s, uint8_t &v)
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

    // Markov transition with power-law-based bias for saturation
    int8_t markovTransitionSaturationBiased(int8_t currentDir, uint8_t currentSat)
    {
        int8_t bias = SAT_BIAS_TABLE[currentSat];
        int roll = random(100);

        // If no prior direction, use bias to determine initial move
        if (currentDir == 0)
        {
            // Bias toward higher saturation (table values are positive = move up)
            if (roll < 40 + bias)
                return 1;  // Move toward higher saturation
            if (roll < 70)
                return 0;  // Stay stationary
            return -1;     // Move toward lower saturation
        }

        // Apply momentum (60% chance to continue) + bias adjustment
        if (currentDir > 0)
        {
            // Currently increasing
            if (roll < 60 + bias / 2)
                return 1;  // Continue increasing
            if (roll < 85)
                return 0;  // Stop
            return -1;     // Reverse
        }
        else
        {
            // Currently decreasing
            if (roll < 60 - bias / 2)
                return -1; // Continue decreasing
            if (roll < 85)
                return 0;  // Stop
            return 1;      // Reverse
        }
    }

    // Shared dark-shimmer constants and methods for inverted base variants
    static constexpr uint8_t DARK_MAX_BRIGHTNESS = 51;          // ~20% of 255
    static constexpr uint16_t DARK_MAX_ACTIVE_LEDS = MAX_LEDS / 5; // 40 = 20% cap

    bool darkBrightSlowToggle = false; // flips every frame, brightness walks only on true

    // Downward bias lookup table (indices 0–51).
    // bias[b] = round(30 × sqrt(b / 51)) — 0 at floor, 30 at ceiling.
    static constexpr int8_t DARK_BRIGHT_BIAS[52] = {
         0,  4,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 15, 16, 16,
        17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23,
        24, 24, 24, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29,
        29, 29, 30, 30
    };

    // Power-law-biased brightness transition toward 0.
    // Higher brightness → stronger downward pull toward 0.
    // Momentum 40%, stationary ~40-55% — slower and smoother than normal base.
    int8_t darkBrightnessTransition(int8_t currentDir, uint8_t currentBright)
    {
        int8_t bias = DARK_BRIGHT_BIAS[min((int)currentBright, (int)DARK_MAX_BRIGHTNESS)];
        int roll = random(100);
        if (currentDir == 0) {
            // 55% stay, 25% down (+ bias), 20% up (- bias)
            if (roll < 25 + bias) return -1;
            if (roll < 80) return 0;
            return 1;
        }
        if (currentDir < 0) {
            // 40% continue, 40% stay, 20% reverse
            if (roll < 40 + bias / 2) return -1;
            if (roll < 80) return 0;
            return 1;
        } else {
            // 40% continue, 40% stay, 20% reverse
            if (roll < 40 - bias / 2) return 1;
            if (roll < 80) return 0;
            return -1;
        }
    }

    // Initialize base layer with dark-biased brightness (call from derived class begin()/reset())
    void initInvertedBaseLayer()
    {
        resetBaseLayer();
        for (int ch = 0; ch < 4; ch++)
        {
            // Zero all brightness first
            for (int i = 0; i < MAX_LEDS; i++)
            {
                baseBrightness[ch][i] = 0;
                brightDir[ch][i] = 0;
            }
            // Randomly activate only DARK_MAX_ACTIVE_LEDS LEDs with cubic power-law brightness
            for (uint16_t n = 0; n < DARK_MAX_ACTIVE_LEDS; n++)
            {
                int i = random(MAX_LEDS);
                float u = random(1000) / 1000.0f;
                baseBrightness[ch][i] = (uint8_t)(DARK_MAX_BRIGHTNESS * powf(u, 3.0f));
            }
        }
    }

    // Update base layer with dark-biased brightness walk (call every frame for inverted variants)
    void updateInvertedBaseLayer()
    {
        darkBrightSlowToggle = !darkBrightSlowToggle;

        for (int ch = 0; ch < 4; ch++)
        {
            // Count currently active LEDs (brightness > 0) for the cap check
            uint16_t activeCount = 0;
            for (int i = 0; i < MAX_LEDS; i++)
            {
                if (baseBrightness[ch][i] > 0) activeCount++;
            }

            for (int i = 0; i < MAX_LEDS; i++)
            {
                bool isActive = baseBrightness[ch][i] > 0;

                if (darkBrightSlowToggle)
                {
                    // Brightness walk — power-law biased toward 0 (every other frame = 2x slower)
                    int8_t nextBrightDir = darkBrightnessTransition(brightDir[ch][i], baseBrightness[ch][i]);

                    if (baseBrightness[ch][i] >= DARK_MAX_BRIGHTNESS && nextBrightDir > 0)
                        nextBrightDir = darkBrightnessTransition(-1, DARK_MAX_BRIGHTNESS);
                    else if (baseBrightness[ch][i] == 0 && nextBrightDir < 0)
                        nextBrightDir = darkBrightnessTransition(1, 0);

                    // Gate: dark LED cannot turn on if at cap
                    if (!isActive && nextBrightDir > 0 && activeCount >= DARK_MAX_ACTIVE_LEDS)
                        nextBrightDir = 0;

                    brightDir[ch][i] = nextBrightDir;
                    // Step 1 (not 2) — 2x slower than before, combined with every-other-frame = 4x
                    baseBrightness[ch][i] = (uint8_t)constrain((int)baseBrightness[ch][i] + nextBrightDir, 0, DARK_MAX_BRIGHTNESS);

                    // Track activeCount as LEDs switch on/off this frame
                    bool nowActive = baseBrightness[ch][i] > 0;
                    if (!isActive && nowActive) activeCount++;
                    else if (isActive && !nowActive) activeCount--;
                }

                // Hue and saturation are fixed — only brightness walks
            }
        }
    }

    // Update base layer undulations (called every frame by derived classes)
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
                baseBrightness[ch][i] = (uint8_t)constrain((int)baseBrightness[ch][i] + nextBrightDir * 2, BASE_BRIGHTNESS, MAX_BRIGHTNESS);

                // Saturation random walk (power-law-biased)
                int8_t nextSatDir = markovTransitionSaturationBiased(satDir[ch][i], baseSaturation[ch][i]);

                // Boundary handling
                if (baseSaturation[ch][i] >= 255 && nextSatDir > 0)
                {
                    nextSatDir = markovTransitionSaturationBiased(-1, 255);
                }
                else if (baseSaturation[ch][i] <= MIN_SATURATION && nextSatDir < 0)
                {
                    nextSatDir = markovTransitionSaturationBiased(1, MIN_SATURATION);
                }

                satDir[ch][i] = nextSatDir;
                baseSaturation[ch][i] = constrain(baseSaturation[ch][i] + nextSatDir * 2, MIN_SATURATION, 255);
            }
        }
    }
};
