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
    // Brightness knock-to-zero effect configuration
    static constexpr uint8_t BRIGHTNESS_KNOCK_ZERO_PCT = 5;  // % chance to knock to 0 at MAX

    // Primary hue desaturation (applies to all harmony animations)
    static constexpr uint8_t PRIMARY_HUE_SAT = 0;  // White when primary hue chosen

    // Helper: Get channel's stored hue value for color-based animations
    // This is set by derived classes via constructor
    int channelHue[4] = {0, 120, 240, 0};  // Default: R, G, B, White

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
