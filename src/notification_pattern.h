#pragma once

#include <Arduino.h>
#include <FastLED.h>

// Notification pattern types
enum NotificationPattern {
    PATTERN_NONE,           // No pattern (restore previous state)
    PATTERN_SOLID,          // Solid color on first 8 LEDs
    PATTERN_SEQUENTIAL,     // Sequential flash through first 8 LEDs
    PATTERN_WARNING         // Warning pattern: blue base, one purple LED cycling
};

// Notification state for all channels
class NotificationState {
public:
    NotificationState() :
        active(false),
        pattern(PATTERN_NONE),
        currentStep(0),
        lastUpdateMs(0),
        stepDurationMs(0),
        cycleCount(0),
        maxCycles(0) {}

    // Start a notification pattern
    void start(NotificationPattern p, CRGB color, uint16_t stepDuration = 100, uint8_t cycles = 0) {
        if (!active) {
            // Save current LED state before starting pattern
            saveState();
        }
        active = true;
        pattern = p;
        primaryColor = color;
        currentStep = 0;
        cycleCount = 0;
        maxCycles = cycles;
        lastUpdateMs = millis();
        stepDurationMs = stepDuration;
    }

    // Stop notification and restore previous state
    void stop() {
        if (active) {
            restoreState();
            active = false;
            pattern = PATTERN_NONE;
        }
    }

    // Update animation (call from loop)
    // Returns true if animation is still running, false if completed
    bool update(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4, uint16_t numLeds) {
        if (!active) return false;

        unsigned long now = millis();
        if (now - lastUpdateMs < stepDurationMs) return true;

        lastUpdateMs = now;

        switch (pattern) {
            case PATTERN_SOLID:
                renderSolid(ch1, ch2, ch3, ch4);
                break;

            case PATTERN_SEQUENTIAL:
                renderSequential(ch1, ch2, ch3, ch4);
                currentStep = (currentStep + 1) % 8;
                if (currentStep == 0 && maxCycles > 0) {
                    cycleCount++;
                    if (cycleCount >= maxCycles) {
                        // Animation complete
                        return false;
                    }
                }
                break;

            case PATTERN_WARNING:
                renderWarning(ch1, ch2, ch3, ch4);
                currentStep = (currentStep + 1) % 8;
                if (currentStep == 0 && maxCycles > 0) {
                    cycleCount++;
                    if (cycleCount >= maxCycles) {
                        // Animation complete
                        return false;
                    }
                }
                break;

            default:
                break;
        }

        return true;
    }

    // Show confirmation pattern (solid color for duration, then restore)
    void showConfirmation(CRGB color, uint16_t durationMs, CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        // Show solid color
        for (int i = 0; i < 8; i++) {
            ch1[i] = color;
            ch2[i] = color;
            ch3[i] = color;
            ch4[i] = color;
        }
        FastLED.show();
        delay(durationMs);

        // Restore previous state
        restoreState();
    }

    bool isActive() const { return active; }

private:
    bool active;
    NotificationPattern pattern;
    CRGB primaryColor;
    CRGB secondaryColor;
    uint8_t currentStep;
    unsigned long lastUpdateMs;
    uint16_t stepDurationMs;
    uint8_t cycleCount;     // Current cycle count (for cycle-limited animations)
    uint8_t maxCycles;      // Maximum cycles (0 = unlimited)

    // Storage for previous LED state (first 8 LEDs of each channel)
    CRGB savedCh1[8];
    CRGB savedCh2[8];
    CRGB savedCh3[8];
    CRGB savedCh4[8];

    void saveState() {
        // Save will happen when first pattern starts
        // Actual saving done in start() by copying from channel arrays
    }

    void restoreState() {
        // Restore happens in caller context where channel arrays are accessible
    }

    void renderSolid(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        for (int i = 0; i < 8; i++) {
            ch1[i] = primaryColor;
            ch2[i] = primaryColor;
            ch3[i] = primaryColor;
            ch4[i] = primaryColor;
        }
    }

    void renderSequential(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        // Clear all first
        for (int i = 0; i < 8; i++) {
            ch1[i] = CRGB::Black;
            ch2[i] = CRGB::Black;
            ch3[i] = CRGB::Black;
            ch4[i] = CRGB::Black;
        }

        // Light up current step
        ch1[currentStep] = primaryColor;
        ch2[currentStep] = primaryColor;
        ch3[currentStep] = primaryColor;
        ch4[currentStep] = primaryColor;
    }

    void renderWarning(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        // Base: All 8 LEDs blue
        CRGB baseColor = CRGB::Blue;
        CRGB highlightColor = CRGB(128, 0, 128); // Purple

        for (int i = 0; i < 8; i++) {
            CRGB color = (i == currentStep) ? highlightColor : baseColor;
            ch1[i] = color;
            ch2[i] = color;
            ch3[i] = color;
            ch4[i] = color;
        }
    }

    // Friend class to allow access to saved state
    friend class NotificationManager;
};

// Manager class to handle state saving/restoration
class NotificationManager {
public:
    NotificationManager(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) :
        channel1(ch1), channel2(ch2), channel3(ch3), channel4(ch4) {}

    void start(NotificationPattern pattern, CRGB color, uint16_t stepDuration = 100, uint8_t cycles = 0) {
        // Save current state
        for (int i = 0; i < 8; i++) {
            state.savedCh1[i] = channel1[i];
            state.savedCh2[i] = channel2[i];
            state.savedCh3[i] = channel3[i];
            state.savedCh4[i] = channel4[i];
        }
        state.start(pattern, color, stepDuration, cycles);
    }

    void stop() {
        if (state.isActive()) {
            // Restore saved state
            for (int i = 0; i < 8; i++) {
                channel1[i] = state.savedCh1[i];
                channel2[i] = state.savedCh2[i];
                channel3[i] = state.savedCh3[i];
                channel4[i] = state.savedCh4[i];
            }
            state.stop();
        }
    }

    bool update(uint16_t numLeds) {
        return state.update(channel1, channel2, channel3, channel4, numLeds);
    }

    // Get cycle count (for tracking animation progress)
    uint8_t getCycleCount() const { return state.cycleCount; }
    uint8_t getMaxCycles() const { return state.maxCycles; }

    void showConfirmation(CRGB color, uint16_t durationMs) {
        state.showConfirmation(color, durationMs, channel1, channel2, channel3, channel4);
    }

    bool isActive() const { return state.isActive(); }

private:
    NotificationState state;
    CRGB* channel1;
    CRGB* channel2;
    CRGB* channel3;
    CRGB* channel4;
};
