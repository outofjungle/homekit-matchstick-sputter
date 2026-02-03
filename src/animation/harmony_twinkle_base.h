#pragma once

#include "animation_base.h"

// Base class for all harmony-based twinkle animations
// Shares LED strip among harmony colors using brightness-based distribution
// Primary hue gets 20-80% of LEDs based on brightness (80% at full bright)
// Each hue includes analogous spread (±5° normal distribution)
//
// Derived classes implement getHarmonyOffsets() and getNumHarmonyHues()
class HarmonyTwinkleBase : public AnimationBase {
public:
    // Tunable parameters (shared across all harmony animations)
    static constexpr uint8_t TWINKLE_DENSITY = 8;    // 1/density chance per frame per LED (higher = fewer twinkles)
    static constexpr uint8_t FADE_SPEED = 15;        // How fast LEDs fade to target (0-255, higher = faster)
    static constexpr unsigned long FRAME_MS = 50;    // Frame update interval (50ms = 20fps)
    static constexpr uint8_t BASE_BRIGHTNESS = 20;   // Minimum brightness when "off"
    static constexpr uint8_t MAX_BRIGHTNESS = 255;   // Maximum brightness when fully lit
    static constexpr int ANGLE_WIDTH = 10;           // Analogous spread range (±5° from target hue)

    HarmonyTwinkleBase() {
        reset();
    }

    // Set channel hues (called by manager when animation starts)
    void setChannelHues(int h1, int h2, int h3, int h4) {
        channelHue[0] = h1;
        channelHue[1] = h2;
        channelHue[2] = h3;
        channelHue[3] = h4;

        // Reassign LED hues with new primary hues
        for (int ch = 0; ch < 4; ch++) {
            assignLedHues(ch, cachedBrightness[ch]);
        }
    }

    // Set channel brightnesses (triggers LED hue reassignment if changed)
    void setChannelBrightnesses(int b1, int b2, int b3, int b4) {
        int brightnesses[4] = {b1, b2, b3, b4};
        for (int ch = 0; ch < 4; ch++) {
            if (brightnesses[ch] != cachedBrightness[ch]) {
                cachedBrightness[ch] = brightnesses[ch];
                assignLedHues(ch, brightnesses[ch]);
            }
        }
    }

    void begin() override {
        reset();

        // Assign LED hues now that derived class is fully constructed
        for (int ch = 0; ch < 4; ch++) {
            assignLedHues(ch, cachedBrightness[ch]);
        }
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
        // Render each channel with its pre-assigned harmony hues
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
                ledHue[ch][i] = 0;  // Will be assigned properly in begin()
            }
            cachedBrightness[ch] = 100;  // Default to full brightness
        }
        frameAccumulator = 0;
    }

protected:
    static constexpr uint16_t MAX_LEDS = 200;

    // Per-LED state (0-255)
    uint8_t currentBrightness[4][MAX_LEDS];
    uint8_t targetBrightness[4][MAX_LEDS];
    uint8_t ledHue[4][MAX_LEDS];  // Pre-assigned hue per LED (FastLED 0-255)

    // Cached channel brightness for change detection
    int cachedBrightness[4];

    // Frame timing
    unsigned long frameAccumulator = 0;

    // Derived classes implement these to define the harmony
    virtual const int* getHarmonyOffsets() const = 0;  // Hue offsets from primary (0°, ...)
    virtual int getNumHarmonyHues() const = 0;         // Number of hues in harmony

    // Generate analogous spread offset using normal distribution approximation
    // Central Limit Theorem: sum of 6 uniform randoms approximates normal distribution
    int generateSpread() {
        int sum = 0;
        for (int i = 0; i < 6; i++) {
            sum += random(0, ANGLE_WIDTH + 1);
        }
        return (sum / 6) - (ANGLE_WIDTH / 2);  // Centered at 0
    }

    // Assign LED hues based on harmony and brightness-based distribution
    // Primary hue gets 20-80% of LEDs (80% at brightness=100)
    // Remaining LEDs divided among secondary hues
    // LEDs are shuffled to randomize positions
    void assignLedHues(uint8_t channelIndex, int brightness) {
        const int* offsets = getHarmonyOffsets();
        int numHues = getNumHarmonyHues();
        int primaryHue360 = channelHue[channelIndex];

        // 1. Calculate primary count from brightness (20% at 0, 80% at 100)
        float primaryPercent = 0.20f + (brightness / 100.0f) * 0.60f;
        int primaryCount = (int)(MAX_LEDS * primaryPercent);

        // 2. Divide remaining LEDs among secondary hues
        int remainingLeds = MAX_LEDS - primaryCount;
        int secondaryCount = (numHues > 1) ? remainingLeds / (numHues - 1) : 0;

        // Handle rounding: give any extra LEDs to primary
        int totalAssigned = primaryCount + (secondaryCount * (numHues - 1));
        if (totalAssigned < MAX_LEDS) {
            primaryCount += (MAX_LEDS - totalAssigned);
        }

        // 3. Assign LEDs sequentially by hue group
        int ledIndex = 0;
        for (int h = 0; h < numHues && ledIndex < MAX_LEDS; h++) {
            int count = (h == 0) ? primaryCount : secondaryCount;
            int hue360 = (primaryHue360 + offsets[h] + 360) % 360;

            for (int i = 0; i < count && ledIndex < MAX_LEDS; i++) {
                int spread = generateSpread();
                int finalHue360 = (hue360 + spread + 360) % 360;
                ledHue[channelIndex][ledIndex] = map(finalHue360, 0, 360, 0, 255);
                ledIndex++;
            }
        }

        // 4. Fisher-Yates shuffle to randomize LED positions
        for (int i = MAX_LEDS - 1; i > 0; i--) {
            int j = random(0, i + 1);
            uint8_t temp = ledHue[channelIndex][i];
            ledHue[channelIndex][i] = ledHue[channelIndex][j];
            ledHue[channelIndex][j] = temp;
        }
    }

private:
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

    // Render twinkle effect for a single channel using pre-assigned hues
    void renderChannel(CRGB* leds, uint16_t numLeds, uint8_t channelIndex) {
        for (int i = 0; i < numLeds; i++) {
            // Use pre-assigned hue with variable brightness
            leds[i] = CHSV(ledHue[channelIndex][i], 255, currentBrightness[channelIndex][i]);
        }
    }
};
