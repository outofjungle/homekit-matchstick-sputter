#pragma once

#include <Arduino.h>
#include <FastLED.h>

// Base class for all ambient animations
// Animations update all 4 channels simultaneously with non-blocking, timer-based updates
class AnimationBase {
public:
    virtual ~AnimationBase() {}

    // Initialize the animation
    // Called when the animation starts
    virtual void begin() = 0;

    // Update animation state (non-blocking)
    // Returns true if animation should continue, false if it should stop
    // deltaMs: milliseconds since last update
    virtual bool update(unsigned long deltaMs) = 0;

    // Render the animation to the LED arrays
    // Called after update() to apply the current frame
    virtual void render(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4, uint16_t numLeds) = 0;

    // Reset animation to initial state
    virtual void reset() = 0;

    // Get animation name (for debugging)
    virtual const char* getName() const = 0;

protected:
    // Common constants shared across animation types
    static constexpr uint16_t MAX_LEDS = 200;
    static constexpr unsigned long FRAME_MS = 50;    // 20fps
    static constexpr int ANGLE_WIDTH = 10;           // ±5° hue spread

    // Brightness knock-to-zero effect configuration
    static constexpr uint8_t BRIGHTNESS_KNOCK_ZERO_PCT = 5;  // % chance to knock to 0 at MAX

    // Primary hue desaturation (applies to all harmony animations)
    static constexpr uint8_t PRIMARY_HUE_SAT = 0;  // White when primary hue chosen

    // Shared channel state (used by most animations)
    int channelHue[4] = {0, 120, 240, 0};      // Default: R, G, B, White
    int cachedBrightness[4] = {100, 100, 100, 100}; // Default: full brightness

    // Frame timing accumulator
    unsigned long frameAccumulator = 0;

    // Set channel hues (called by manager when animation starts or hue changes)
    // Default implementation - derived classes can override
    virtual void setChannelHues(int h1, int h2, int h3, int h4) {
        channelHue[0] = h1;
        channelHue[1] = h2;
        channelHue[2] = h3;
        channelHue[3] = h4;
    }

    // Set channel brightnesses (called by manager when brightness changes)
    // Default implementation - derived classes can override
    virtual void setChannelBrightnesses(int b1, int b2, int b3, int b4) {
        cachedBrightness[0] = b1;
        cachedBrightness[1] = b2;
        cachedBrightness[2] = b3;
        cachedBrightness[3] = b4;
    }

    // Generate analogous spread offset using normal distribution approximation
    int generateSpread() {
        int sum = 0;
        for (int i = 0; i < 6; i++) {
            sum += random(0, ANGLE_WIDTH + 1);
        }
        return (sum / 6) - (ANGLE_WIDTH / 2); // Centered at 0
    }

    // Markov chain transition: returns -1, 0, or +1
    // Momentum: 60% chance to continue current direction
    int markovTransition(int8_t currentDir) {
        int roll = random(100);

        if (currentDir == 0) {
            // No prior direction: equal probability
            if (roll < 33) return -1;
            if (roll < 67) return 0;
            return 1;
        }
        else if (currentDir > 0) {
            // Moving positive: 60% stay positive, 20% stay, 20% reverse
            if (roll < 60) return 1;
            if (roll < 80) return 0;
            return -1;
        }
        else {
            // Moving negative: 60% stay negative, 20% stay, 20% reverse
            if (roll < 60) return -1;
            if (roll < 80) return 0;
            return 1;
        }
    }

    // Markov chain transition with upward bias (towards brightness)
    // Returns -1 (decrease), 0 (stay), or +1 (increase)
    // Biased to favor increasing values (brighter) over decreasing
    int markovTransitionBrightnessBiased(int8_t currentDir) {
        int roll = random(100);

        if (currentDir == 0) {
            // No prior direction: 60% up, 20% stay, 20% down
            if (roll < 60) return 1;   // Bias towards increasing
            if (roll < 80) return 0;
            return -1;
        }
        else if (currentDir > 0) {
            // Moving up: 70% continue up, 15% stay, 15% reverse down
            if (roll < 70) return 1;   // Strong momentum upward
            if (roll < 85) return 0;
            return -1;
        }
        else {
            // Moving down: 40% continue down, 30% stay, 30% reverse up
            if (roll < 40) return -1;  // Weaker momentum downward
            if (roll < 70) return 0;
            return 1;                  // More likely to reverse
        }
    }
};
