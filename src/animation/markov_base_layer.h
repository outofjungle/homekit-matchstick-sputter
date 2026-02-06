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

protected:
    // Per-LED base state (4 channels × MAX_LEDS)
    int8_t hueOffset[4][MAX_LEDS];       // Current offset from channel hue (-ANGLE_WIDTH/2 to +ANGLE_WIDTH/2)
    int8_t hueDir[4][MAX_LEDS];          // Last hue move direction: -1, 0, +1
    uint8_t baseBrightness[4][MAX_LEDS]; // Current base brightness
    int8_t brightDir[4][MAX_LEDS];       // Last brightness move direction: -1, 0, +1

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
                baseBrightness[ch][i] += nextBrightDir * 2; // Step by 2
                baseBrightness[ch][i] = constrain(baseBrightness[ch][i], BASE_BRIGHTNESS, MAX_BRIGHTNESS);
            }
        }
    }
};
