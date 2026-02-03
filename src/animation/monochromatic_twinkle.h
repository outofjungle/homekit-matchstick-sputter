#pragma once

#include "animation_base.h"

// Monochromatic Twinkle Effect Animation
// Random LEDs fade in and out at different rates, creating a sparkling effect
// Uses analogous spread (±5° around primary hue) for visual variety
//
// Algorithm:
// 1. Each LED has a target brightness and current brightness
// 2. Each frame:
//    - Some LEDs get new random target brightness (twinkle density)
//    - All LEDs fade toward their target brightness (fade speed)
// 3. Color based on channel's stored hue (from HomeKit state) with analogous spread
class MonochromaticTwinkle : public AnimationBase {
public:
    // Tunable parameters
    static constexpr uint8_t TWINKLE_DENSITY = 8;    // 1/density chance per frame per LED (higher = fewer twinkles)
    static constexpr uint8_t FADE_SPEED = 15;        // How fast LEDs fade to target (0-255, higher = faster)
    static constexpr unsigned long FRAME_MS = 50;    // Frame update interval (50ms = 20fps)
    static constexpr uint8_t BASE_BRIGHTNESS = 20;   // Minimum brightness when "off"
    static constexpr uint8_t MAX_BRIGHTNESS = 255;   // Maximum brightness when fully lit
    static constexpr int ANGLE_WIDTH = 10;           // Analogous spread range (±5° from target hue)

    MonochromaticTwinkle() {
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
            updateState();
            return true;  // Update needed
        }

        return false;  // No update needed yet
    }

    void render(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4, uint16_t numLeds) override {
        // Render each channel with its stored hue
        renderChannel(ch1, numLeds, 0);
        renderChannel(ch2, numLeds, 1);
        renderChannel(ch3, numLeds, 2);
        renderChannel(ch4, numLeds, 3);
    }

    void reset() override {
        // Initialize all LEDs to base brightness
        for (int ch = 0; ch < 4; ch++) {
            for (int i = 0; i < MAX_LEDS; i++) {
                currentBrightness[ch][i] = BASE_BRIGHTNESS;
                targetBrightness[ch][i] = BASE_BRIGHTNESS;
            }
        }
        frameAccumulator = 0;
    }

    const char* getName() const override {
        return "Monochromatic Twinkle";
    }

private:
    static constexpr uint16_t MAX_LEDS = 200;

    // Per-LED brightness state (0-255)
    uint8_t currentBrightness[4][MAX_LEDS];
    uint8_t targetBrightness[4][MAX_LEDS];

    // Frame timing
    unsigned long frameAccumulator = 0;

    // Update brightness state for all channels
    void updateState() {
        for (int ch = 0; ch < 4; ch++) {
            for (int i = 0; i < MAX_LEDS; i++) {
                // Random chance to assign new target brightness
                if (random(TWINKLE_DENSITY) == 0) {
                    // Pick a random brightness between BASE and MAX
                    targetBrightness[ch][i] = random(BASE_BRIGHTNESS, MAX_BRIGHTNESS);
                }

                // Fade current brightness toward target
                if (currentBrightness[ch][i] < targetBrightness[ch][i]) {
                    currentBrightness[ch][i] = qadd8(currentBrightness[ch][i], FADE_SPEED);
                    if (currentBrightness[ch][i] > targetBrightness[ch][i]) {
                        currentBrightness[ch][i] = targetBrightness[ch][i];
                    }
                } else if (currentBrightness[ch][i] > targetBrightness[ch][i]) {
                    currentBrightness[ch][i] = qsub8(currentBrightness[ch][i], FADE_SPEED);
                    if (currentBrightness[ch][i] < targetBrightness[ch][i]) {
                        currentBrightness[ch][i] = targetBrightness[ch][i];
                    }
                }
            }
        }
    }

    // Generate analogous spread offset using normal distribution approximation
    // Central Limit Theorem: sum of 6 uniform randoms approximates normal distribution
    int generateSpread() {
        int sum = 0;
        for (int i = 0; i < 6; i++) {
            sum += random(0, ANGLE_WIDTH + 1);
        }
        return (sum / 6) - (ANGLE_WIDTH / 2);  // Centered at 0
    }

    // Render twinkle effect for a single channel
    void renderChannel(CRGB* leds, uint16_t numLeds, uint8_t channelIndex) {
        // Convert HomeKit hue (0-360) to base hue
        int baseHue360 = channelHue[channelIndex];

        for (int i = 0; i < numLeds; i++) {
            // Apply analogous spread to each LED
            int spread = generateSpread();
            int finalHue360 = (baseHue360 + spread + 360) % 360;
            uint8_t hue8 = map(finalHue360, 0, 360, 0, 255);

            // Use channel's hue with spread, full saturation, variable brightness
            leds[i] = CHSV(hue8, 255, currentBrightness[channelIndex][i]);
        }
    }
};
