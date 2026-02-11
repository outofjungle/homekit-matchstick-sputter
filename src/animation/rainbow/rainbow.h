#pragma once

#include "../animation_base.h"

// Rainbow animation: full-spectrum hue sweep along each strip.
// No relation to channel primary color — pure rainbow.
//
// Brightness controls both speed and band width:
//   low  brightness = slow scroll + wide color bands
//   high brightness = fast scroll + narrow bands
//
// Parameters derived from per-channel cachedBrightness[ch] (0-100):
//   deltaHue: 1 + (brightness * 9 / 100)   → range 1..10 (hue increment per LED)
//   speed:    0.2 + (brightness * 2.8 / 100) → range 0.2..3.0 (phase advance per frame)
class RainbowAnimation : public AnimationBase {
public:
    void begin() override {
        reset();
    }

    void reset() override {
        for (int i = 0; i < 4; i++) {
            phaseOffset[i] = 0.0f;
        }
        frameAccumulator = 0;
    }

    bool update(unsigned long deltaMs) override {
        frameAccumulator += deltaMs;
        if (frameAccumulator < FRAME_MS) {
            return false;
        }
        frameAccumulator -= FRAME_MS;

        for (int ch = 0; ch < 4; ch++) {
            float speed = 6.0f + (cachedBrightness[ch] * 3.5f / 100.0f);
            phaseOffset[ch] += speed;
            if (phaseOffset[ch] >= 256.0f) {
                phaseOffset[ch] -= 256.0f;
            }
        }

        return true;
    }

    void render(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4, uint16_t numLeds) override {
        CRGB* channels[4] = {ch1, ch2, ch3, ch4};
        for (int ch = 0; ch < 4; ch++) {
            uint8_t startHue = (uint8_t)phaseOffset[ch];
            fill_rainbow(channels[ch], numLeds, startHue, 1);
        }
    }

    const char* getName() const override {
        return "Rainbow";
    }

private:
    float phaseOffset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};
