#pragma once

#include "../markov_base_layer.h"

// Base Only Animation
// Displays only the MarkovBaseLayer breathing effect (no overlay animations)
// Single mode — no harmony variants since base layer uses channel hue only
class BaseOnlyAnimation : public MarkovBaseLayer
{
public:
    const char *getName() const override
    {
        return "Base";
    }

    void begin() override
    {
        resetBaseLayer();
        frameAccumulator = 0;
    }

    bool update(unsigned long deltaMs) override
    {
        frameAccumulator += deltaMs;

        if (frameAccumulator >= FRAME_MS)
        {
            frameAccumulator -= FRAME_MS;
            updateBaseLayer();
            return true;
        }

        return false;
    }

    void render(CRGB *ch1, CRGB *ch2, CRGB *ch3, CRGB *ch4, uint16_t numLeds) override
    {
        renderBaseChannel(ch1, numLeds, 0);
        renderBaseChannel(ch2, numLeds, 1);
        renderBaseChannel(ch3, numLeds, 2);
        renderBaseChannel(ch4, numLeds, 3);
    }

    void reset() override
    {
        resetBaseLayer();
        frameAccumulator = 0;
    }

protected:
    const int *getHarmonyOffsets() const override
    {
        static const int offsets[] = {0};
        return offsets;
    }

    int getNumHarmonyHues() const override
    {
        return 1;
    }

private:
    void renderBaseChannel(CRGB *leds, uint16_t numLeds, uint8_t channelIndex)
    {
        for (int i = 0; i < (int)numLeds; i++)
        {
            leds[i] = getBaseLedColor(channelIndex, i);
        }
    }
};
