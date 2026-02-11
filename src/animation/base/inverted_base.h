#pragma once

#include "base_only.h"

// Inverted Base Animation
// Dark shimmer: hue and saturation walk identically to the normal base layer,
// but brightness is power-law-biased toward near-black. Most LEDs stay under
// ~5% brightness (13/255); occasional drift up to DARK_MAX_BRIGHTNESS (~20%).
// No bright flares — the strip stays dim at all times.
class InvertedBaseAnimation : public BaseOnlyAnimation
{
    static constexpr uint8_t DARK_MAX_BRIGHTNESS = 51; // ~20% of 255

    // Downward bias lookup table (indices 0–51).
    // bias[b] = round(30 × sqrt(b / 51)) — 0 at floor, 30 at ceiling.
    // Higher bias pulls the random walk back toward 0.
    static constexpr int8_t DARK_BRIGHT_BIAS[DARK_MAX_BRIGHTNESS + 1] = {
         0,  4,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 15, 16, 16,
        17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23,
        24, 24, 24, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29,
        29, 29, 30, 30
    };

public:
    const char *getName() const override
    {
        return "Inverted Base";
    }

    void begin() override
    {
        resetBaseLayer();
        // Power-law-distributed initial brightness, concentrated near 0.
        // With α=3: ~63% of LEDs start below brightness 13 (~5% of 255).
        for (int ch = 0; ch < 4; ch++)
        {
            for (int i = 0; i < MAX_LEDS; i++)
            {
                float u = random(1000) / 1000.0f;
                baseBrightness[ch][i] = (uint8_t)(DARK_MAX_BRIGHTNESS * powf(u, 3.0f));
                brightDir[ch][i] = 0;
            }
        }
        frameAccumulator = 0;
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;

        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateInvertedLayer();
            return true;
        }

        return false;
    }

    void reset() override
    {
        begin();
    }

    void render(CRGB *ch1, CRGB *ch2, CRGB *ch3, CRGB *ch4, uint16_t numLeds) override
    {
        renderDarkChannel(ch1, numLeds, 0);
        renderDarkChannel(ch2, numLeds, 1);
        renderDarkChannel(ch3, numLeds, 2);
        renderDarkChannel(ch4, numLeds, 3);
    }

private:
    // Power-law-biased brightness transition.
    // Higher brightness → stronger downward pull toward 0.
    int8_t darkBrightnessTransition(int8_t currentDir, uint8_t currentBright)
    {
        int8_t bias = DARK_BRIGHT_BIAS[min((int)currentBright, (int)DARK_MAX_BRIGHTNESS)];
        int roll = random(100);
        if (currentDir == 0) {
            if (roll < 40 + bias) return -1;  // Bias pushes DOWN
            if (roll < 70) return 0;
            return 1;
        }
        if (currentDir < 0) {  // Currently decreasing (toward dark)
            if (roll < 60 + bias / 2) return -1;
            if (roll < 85) return 0;
            return 1;
        }
        else {  // Currently increasing (toward bright)
            if (roll < 60 - bias / 2) return 1;
            if (roll < 85) return 0;
            return -1;
        }
    }

    void updateInvertedLayer()
    {
        for (int ch = 0; ch < 4; ch++)
        {
            for (int i = 0; i < MAX_LEDS; i++)
            {
                // Hue random walk (identical to base layer)
                int8_t nextHueDir = markovTransition(hueDir[ch][i]);

                if (hueOffset[ch][i] >= ANGLE_WIDTH / 2 && nextHueDir > 0)
                    nextHueDir = markovTransition(-1);
                else if (hueOffset[ch][i] <= -ANGLE_WIDTH / 2 && nextHueDir < 0)
                    nextHueDir = markovTransition(1);

                hueDir[ch][i] = nextHueDir;
                hueOffset[ch][i] += nextHueDir;
                hueOffset[ch][i] = constrain(hueOffset[ch][i], -ANGLE_WIDTH / 2, ANGLE_WIDTH / 2);

                // Brightness random walk — power-law biased toward 0
                int8_t nextBrightDir = darkBrightnessTransition(brightDir[ch][i], baseBrightness[ch][i]);

                if (baseBrightness[ch][i] >= DARK_MAX_BRIGHTNESS && nextBrightDir > 0)
                    nextBrightDir = darkBrightnessTransition(-1, DARK_MAX_BRIGHTNESS);
                else if (baseBrightness[ch][i] == 0 && nextBrightDir < 0)
                    nextBrightDir = darkBrightnessTransition(1, 0);

                brightDir[ch][i] = nextBrightDir;
                baseBrightness[ch][i] = (uint8_t)constrain((int)baseBrightness[ch][i] + nextBrightDir * 2, 0, DARK_MAX_BRIGHTNESS);

                // Saturation random walk (identical to base layer)
                int8_t nextSatDir = markovTransitionSaturationBiased(satDir[ch][i], baseSaturation[ch][i]);

                if (baseSaturation[ch][i] >= 255 && nextSatDir > 0)
                    nextSatDir = markovTransitionSaturationBiased(-1, 255);
                else if (baseSaturation[ch][i] <= MIN_SATURATION && nextSatDir < 0)
                    nextSatDir = markovTransitionSaturationBiased(1, MIN_SATURATION);

                satDir[ch][i] = nextSatDir;
                baseSaturation[ch][i] = (uint8_t)constrain((int)baseSaturation[ch][i] + nextSatDir * 2, MIN_SATURATION, 255);
            }
        }
    }

    void renderDarkChannel(CRGB *leds, uint16_t numLeds, uint8_t channelIndex)
    {
        for (int i = 0; i < (int)numLeds; i++)
        {
            int hue360 = (channelHue[channelIndex] + hueOffset[channelIndex][i] + 360) % 360;
            uint8_t hue8 = map(hue360, 0, 360, 0, 255);
            leds[i] = CHSV(hue8, baseSaturation[channelIndex][i], baseBrightness[channelIndex][i]);
        }
    }
};
