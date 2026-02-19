#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "pairing_config.h"

// Forward declaration
struct DEV_LedChannel;

// Number of LEDs used for notification patterns (first N LEDs of each channel)
static constexpr uint8_t NOTIFICATION_LEDS = 8;
static_assert(NOTIFICATION_LEDS >= 8, "NOTIFICATION_LEDS must be >= 8 for PAIRING_CONFIG_ID binary rendering");
static_assert(PAIRING_CONFIG_ID <= 255, "PAIRING_CONFIG_ID must fit in 8 bits for binary LED rendering");

// Notification pattern types
enum class NotificationPattern {
    PATTERN_NONE,           // No pattern (restore previous state)
    PATTERN_SOLID,          // Solid color on first NOTIFICATION_LEDS LEDs
    PATTERN_SEQUENTIAL,     // Sequential flash through first NOTIFICATION_LEDS LEDs
    PATTERN_WARNING,        // Warning pattern: blue base, one purple LED cycling
    PATTERN_PAIRING_ID,     // Static binary display of PAIRING_CONFIG_ID on LEDs 0-7
    PATTERN_PAIRING_ID_BLINK // Blinking binary display (for identify use)
};

// Notification state for all channels
class NotificationState {
public:
    NotificationState() :
        active(false),
        pattern(NotificationPattern::PATTERN_NONE),
        currentStep(0),
        lastUpdateMs(0),
        stepDurationMs(0),
        cycleCount(0),
        maxCycles(0) {}

    // Start a notification pattern
    void start(NotificationPattern p, CRGB color, uint16_t stepDuration = 100, uint8_t cycles = 0) {
        active = true;
        pattern = p;
        primaryColor = color;
        currentStep = 0;
        cycleCount = 0;
        maxCycles = cycles;
        lastUpdateMs = millis();
        stepDurationMs = stepDuration;
    }

    // Stop notification
    void stop() {
        active = false;
        pattern = NotificationPattern::PATTERN_NONE;
    }

    // Update animation (call from loop)
    // Returns true if animation is still running, false if completed
    bool update(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        if (!active) return false;

        unsigned long now = millis();
        if (now - lastUpdateMs < stepDurationMs) return true;

        lastUpdateMs = now;

        switch (pattern) {
            case NotificationPattern::PATTERN_SOLID:
                renderSolid(ch1, ch2, ch3, ch4);
                break;

            case NotificationPattern::PATTERN_SEQUENTIAL:
                renderSequential(ch1, ch2, ch3, ch4);
                currentStep = (currentStep + 1) % NOTIFICATION_LEDS;
                if (currentStep == 0 && maxCycles > 0) {
                    cycleCount++;
                    if (cycleCount >= maxCycles) {
                        return false;
                    }
                }
                break;

            case NotificationPattern::PATTERN_WARNING:
                renderWarning(ch1, ch2, ch3, ch4);
                currentStep = (currentStep + 1) % NOTIFICATION_LEDS;
                if (currentStep == 0 && maxCycles > 0) {
                    cycleCount++;
                    if (cycleCount >= maxCycles) {
                        return false;
                    }
                }
                break;

            case NotificationPattern::PATTERN_PAIRING_ID:
                renderPairingId(ch1, ch2, ch3, ch4);
                // Static — no stepping
                break;

            case NotificationPattern::PATTERN_PAIRING_ID_BLINK:
                renderPairingIdBlink(ch1, ch2, ch3, ch4);
                currentStep = (currentStep + 1) % 2;
                if (currentStep == 0 && maxCycles > 0) {
                    cycleCount++;
                    if (cycleCount >= maxCycles) {
                        return false;
                    }
                }
                break;

            default:
                // PATTERN_NONE only occurs when !active (guarded at top of update()); unreachable
                break;
        }

        return true;
    }

    bool isActive() const { return active; }

private:
    bool active;
    NotificationPattern pattern;
    CRGB primaryColor;
    uint8_t currentStep;
    unsigned long lastUpdateMs;
    uint16_t stepDurationMs;
    uint8_t cycleCount;     // Current cycle count (for cycle-limited animations)
    uint8_t maxCycles;      // Maximum cycles (0 = unlimited)

    // Storage for previous LED state (first NOTIFICATION_LEDS LEDs of each channel)
    CRGB savedCh1[NOTIFICATION_LEDS];
    CRGB savedCh2[NOTIFICATION_LEDS];
    CRGB savedCh3[NOTIFICATION_LEDS];
    CRGB savedCh4[NOTIFICATION_LEDS];

    void renderSolid(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        for (int i = 0; i < NOTIFICATION_LEDS; i++) {
            ch1[i] = primaryColor;
            ch2[i] = primaryColor;
            ch3[i] = primaryColor;
            ch4[i] = primaryColor;
        }
    }

    void renderSequential(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        // Clear all first
        for (int i = 0; i < NOTIFICATION_LEDS; i++) {
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
        CRGB baseColor = CRGB::Blue;
        CRGB highlightColor = CRGB(128, 0, 128); // Purple

        for (int i = 0; i < NOTIFICATION_LEDS; i++) {
            CRGB color = (i == currentStep) ? highlightColor : baseColor;
            ch1[i] = color;
            ch2[i] = color;
            ch3[i] = color;
            ch4[i] = color;
        }
    }

    // Display PAIRING_CONFIG_ID as binary on LEDs 0-7
    // Bit 0 → LED[0] (LSB), bit 7 → LED[7] (MSB)
    // Bit=0: blue at 50%, Bit=1: purple at 100%
    void renderPairingId(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        static const CRGB bitOff(0, 0, 128);     // Blue at 50%
        static const CRGB bitOn(255, 0, 0);      // Red at 100%

        for (int i = 0; i < NOTIFICATION_LEDS; i++) {
            CRGB color = ((PAIRING_CONFIG_ID >> i) & 0x01) ? bitOn : bitOff;
            ch1[i] = color;
            ch2[i] = color;
            ch3[i] = color;
            ch4[i] = color;
        }
    }

    void renderPairingIdBlink(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) {
        if (currentStep % 2 == 0) {
            renderPairingId(ch1, ch2, ch3, ch4);
        } else {
            for (int i = 0; i < NOTIFICATION_LEDS; i++) {
                ch1[i] = CRGB::Black;
                ch2[i] = CRGB::Black;
                ch3[i] = CRGB::Black;
                ch4[i] = CRGB::Black;
            }
        }
    }

    // Friend class to allow access to saved state
    friend class NotificationManager;
};

// Manager class to handle state saving/restoration
class NotificationManager {
public:
    NotificationManager(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4) :
        channel1(ch1), channel2(ch2), channel3(ch3), channel4(ch4),
        channelService1(nullptr), channelService2(nullptr),
        channelService3(nullptr), channelService4(nullptr) {}

    // Set channel service pointers (call after channel services are created)
    void setChannelServices(DEV_LedChannel* ch1, DEV_LedChannel* ch2, DEV_LedChannel* ch3, DEV_LedChannel* ch4) {
        channelService1 = ch1;
        channelService2 = ch2;
        channelService3 = ch3;
        channelService4 = ch4;
    }

    void start(NotificationPattern pattern, CRGB color, uint16_t stepDuration = 100, uint8_t cycles = 0) {
        // Tell all channel services to yield to notification
        if (channelService1) channelService1->yieldToNotification();
        if (channelService2) channelService2->yieldToNotification();
        if (channelService3) channelService3->yieldToNotification();
        if (channelService4) channelService4->yieldToNotification();

        // Save current state
        for (int i = 0; i < NOTIFICATION_LEDS; i++) {
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
            for (int i = 0; i < NOTIFICATION_LEDS; i++) {
                channel1[i] = state.savedCh1[i];
                channel2[i] = state.savedCh2[i];
                channel3[i] = state.savedCh3[i];
                channel4[i] = state.savedCh4[i];
            }
            state.stop();

            // Tell all channel services to resume from notification
            if (channelService1) channelService1->resumeFromNotification();
            if (channelService2) channelService2->resumeFromNotification();
            if (channelService3) channelService3->resumeFromNotification();
            if (channelService4) channelService4->resumeFromNotification();
        }
    }

    bool update() {
        return state.update(channel1, channel2, channel3, channel4);
    }

    // Get cycle count (for tracking animation progress)
    uint8_t getCycleCount() const { return state.cycleCount; }
    uint8_t getMaxCycles() const { return state.maxCycles; }

    bool isActive() const { return state.isActive(); }

private:
    NotificationState state;
    CRGB* channel1;
    CRGB* channel2;
    CRGB* channel3;
    CRGB* channel4;
    DEV_LedChannel* channelService1;
    DEV_LedChannel* channelService2;
    DEV_LedChannel* channelService3;
    DEV_LedChannel* channelService4;
};
