#pragma once

#include "animation_base.h"

// Fire Effect Animation (Hue-Based)
// Simulates flickering flames using a heat map that rises upward
// Uses each channel's HomeKit hue as the primary flame color
//
// Algorithm:
// 1. Each LED has a heat value (0-255)
// 2. Each frame:
//    - Cool down: reduce heat by random amount
//    - Spark: randomly increase heat at bottom LEDs
//    - Diffuse: heat rises upward (average with neighbors)
// 3. Map heat to HSV color based on channel's hue:
//    - Low heat (0-85): black -> dark saturated hue (V ramps up)
//    - Medium heat (86-170): saturated hue -> bright hue (V maxes out)
//    - High heat (171-255): saturated -> desaturated/white (S ramps down)
class FireAnimation : public AnimationBase {
public:
    // Tunable parameters
    static constexpr uint8_t COOLING = 55;        // How much to cool down each frame (higher = faster cooling)
    static constexpr uint8_t SPARKING = 120;      // Probability of new sparks (0-255, higher = more sparks)
    static constexpr unsigned long FRAME_MS = 50; // Frame update interval (50ms = 20fps)

    FireAnimation() {
        reset();
    }

    // Set channel hues (called by manager when animation starts)
    void setChannelHues(int h1, int h2, int h3, int h4) {
        channelHue[0] = h1;
        channelHue[1] = h2;
        channelHue[2] = h3;
        channelHue[3] = h4;
    }

    void begin() override {
        reset();
    }

    bool update(unsigned long deltaMs) override {
        frameAccumulator += deltaMs;

        if (frameAccumulator >= FRAME_MS) {
            frameAccumulator -= FRAME_MS;
            return true;  // Update needed
        }

        return false;  // No update needed yet
    }

    void render(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4, uint16_t numLeds) override {
        // Process each channel with its own hue
        renderChannel(ch1, numLeds, 0);
        renderChannel(ch2, numLeds, 1);
        renderChannel(ch3, numLeds, 2);
        renderChannel(ch4, numLeds, 3);
    }

    void reset() override {
        // Clear all heat maps
        for (int ch = 0; ch < 4; ch++) {
            for (int i = 0; i < MAX_LEDS; i++) {
                heat[ch][i] = 0;
            }
        }
        frameAccumulator = 0;
    }

    const char* getName() const override {
        return "Fire";
    }

private:
    static constexpr uint16_t MAX_LEDS = 200;

    // Heat map for each channel (0-255 per LED)
    uint8_t heat[4][MAX_LEDS];

    // Frame timing
    unsigned long frameAccumulator = 0;

    // Render fire effect for a single channel
    void renderChannel(CRGB* leds, uint16_t numLeds, uint8_t channelIndex) {
        // Step 1: Cool down every LED
        for (int i = 0; i < numLeds; i++) {
            uint8_t cooldown = random(0, ((COOLING * 10) / numLeds) + 2);
            if (cooldown > heat[channelIndex][i]) {
                heat[channelIndex][i] = 0;
            } else {
                heat[channelIndex][i] -= cooldown;
            }
        }

        // Step 2: Heat diffuses upward (heat from lower LEDs rises)
        for (int i = numLeds - 1; i >= 2; i--) {
            heat[channelIndex][i] = (heat[channelIndex][i - 1] + heat[channelIndex][i - 2] + heat[channelIndex][i - 2]) / 3;
        }

        // Step 3: Randomly ignite new sparks at the bottom
        if (random(255) < SPARKING) {
            int pos = random(7);  // Spark in bottom 7 LEDs
            heat[channelIndex][pos] = qadd8(heat[channelIndex][pos], random(160, 255));
        }

        // Step 4: Convert heat to LED colors using channel's hue
        uint8_t hue8 = map(channelHue[channelIndex], 0, 360, 0, 255);
        for (int i = 0; i < numLeds; i++) {
            leds[i] = heatToColor(heat[channelIndex][i], hue8);
        }
    }

    // Map heat value to color using channel's hue
    // Heat 0-255 maps through HSV space:
    //   0-85:   Black to dark saturated hue (V: 0->255, S: 255)
    //   86-170: Full saturated hue (V: 255, S: 255) - brightest colored
    //   171-255: Saturated to white (V: 255, S: 255->0)
    CRGB heatToColor(uint8_t temperature, uint8_t hue) {
        uint8_t saturation, value;

        if (temperature < 86) {
            // Cold to warm: ramp up brightness, full saturation
            // Map 0-85 to V: 0-255
            value = map(temperature, 0, 85, 0, 255);
            saturation = 255;
        } else if (temperature < 171) {
            // Warm: full brightness, full saturation (most vivid color)
            value = 255;
            saturation = 255;
        } else {
            // Hot to white: full brightness, desaturate toward white
            // Map 171-255 to S: 255->0
            value = 255;
            saturation = map(temperature, 171, 255, 255, 0);
        }

        return CHSV(hue, saturation, value);
    }
};
