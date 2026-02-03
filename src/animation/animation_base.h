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
    // Helper: Get channel's stored hue value for color-based animations
    // This is set by derived classes via constructor
    int channelHue[4] = {0, 120, 240, 0};  // Default: R, G, B, White
};
