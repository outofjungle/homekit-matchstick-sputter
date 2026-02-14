#pragma once

#include <Preferences.h>
#include "animation_base.h"
#include "runner/monochromatic_runner.h"
#include "runner/complementary_runner.h"
#include "runner/split_complementary_runner.h"
#include "runner/triadic_runner.h"
#include "runner/square_runner.h"
#include "runner/inverted_runner_base.h"
#include "runner/inverted_monochromatic_runner.h"
#include "runner/inverted_complementary_runner.h"
#include "runner/inverted_split_complementary_runner.h"
#include "runner/inverted_triadic_runner.h"
#include "runner/inverted_square_runner.h"
#include "twinkle/monochromatic_twinkle.h"
#include "twinkle/complementary_twinkle.h"
#include "twinkle/split_complementary_twinkle.h"
#include "twinkle/triadic_twinkle.h"
#include "twinkle/square_twinkle.h"
#include "twinkle/inverted_twinkle_base.h"
#include "twinkle/inverted_monochromatic_twinkle.h"
#include "twinkle/inverted_complementary_twinkle.h"
#include "twinkle/inverted_split_complementary_twinkle.h"
#include "twinkle/inverted_triadic_twinkle.h"
#include "twinkle/inverted_square_twinkle.h"
#include "rain/monochromatic_rain.h"
#include "rain/complementary_rain.h"
#include "rain/split_complementary_rain.h"
#include "rain/triadic_rain.h"
#include "rain/square_rain.h"
#include "rain/inverted_rain_base.h"
#include "rain/inverted_monochromatic_rain.h"
#include "rain/inverted_complementary_rain.h"
#include "rain/inverted_split_complementary_rain.h"
#include "rain/inverted_triadic_rain.h"
#include "rain/inverted_square_rain.h"
#include "base/base_only.h"
#include "base/inverted_base.h"
#include "rainbow/rainbow.h"
#include "../led_channel.h"

// Animation modes
enum class AnimationMode {
    ANIM_NONE,                                    // HomeKit control (normal operation)
    // --- Inverted ---
    ANIM_INVERTED_BASE,                           // Inverted base layer (dark shimmer)
    ANIM_INVERTED_MONOCHROMATIC_RUNNER,           // Inverted monochromatic runner
    ANIM_INVERTED_MONOCHROMATIC_RAIN,             // Inverted monochromatic rain
    ANIM_INVERTED_MONOCHROMATIC_TWINKLE,          // Inverted monochromatic twinkle
    ANIM_INVERTED_COMPLEMENTARY_RUNNER,           // Inverted complementary runner
    ANIM_INVERTED_COMPLEMENTARY_RAIN,             // Inverted complementary rain
    ANIM_INVERTED_COMPLEMENTARY_TWINKLE,          // Inverted complementary twinkle
    ANIM_INVERTED_SPLIT_COMPLEMENTARY_RUNNER,     // Inverted split-complementary runner
    ANIM_INVERTED_SPLIT_COMPLEMENTARY_RAIN,       // Inverted split-complementary rain
    ANIM_INVERTED_SPLIT_COMPLEMENTARY_TWINKLE,    // Inverted split-complementary twinkle
    ANIM_INVERTED_TRIADIC_RUNNER,                 // Inverted triadic runner
    ANIM_INVERTED_TRIADIC_RAIN,                   // Inverted triadic rain
    ANIM_INVERTED_TRIADIC_TWINKLE,                // Inverted triadic twinkle
    ANIM_INVERTED_SQUARE_RUNNER,                  // Inverted square runner
    ANIM_INVERTED_SQUARE_RAIN,                    // Inverted square rain
    ANIM_INVERTED_SQUARE_TWINKLE,                 // Inverted square twinkle
    // --- Normal ---
    ANIM_BASE,                                    // Base layer only (no overlay)
    ANIM_MONOCHROMATIC_RUNNER,                    // Monochromatic runner
    ANIM_MONOCHROMATIC_RAIN,                      // Monochromatic rain (primary + white)
    ANIM_MONOCHROMATIC_TWINKLE,                   // Monochromatic twinkle
    ANIM_COMPLEMENTARY_RUNNER,                    // Complementary runner (2 colors)
    ANIM_COMPLEMENTARY_RAIN,                      // Complementary rain (2 colors)
    ANIM_COMPLEMENTARY_TWINKLE,                   // Complementary twinkle (2 colors)
    ANIM_SPLIT_COMPLEMENTARY_RUNNER,              // Split-complementary runner (3 colors)
    ANIM_SPLIT_COMPLEMENTARY_RAIN,                // Split-complementary rain (3 colors)
    ANIM_SPLIT_COMPLEMENTARY_TWINKLE,             // Split-complementary twinkle (3 colors)
    ANIM_TRIADIC_RUNNER,                          // Triadic runner (3 colors)
    ANIM_TRIADIC_RAIN,                            // Triadic rain (3 colors)
    ANIM_TRIADIC_TWINKLE,                         // Triadic twinkle (3 colors)
    ANIM_SQUARE_RUNNER,                           // Square runner (4 colors)
    ANIM_SQUARE_RAIN,                             // Square rain (4 colors)
    ANIM_SQUARE_TWINKLE,                          // Square twinkle (4 colors)
    ANIM_RAINBOW,                                 // Full-spectrum rainbow sweep
    ANIM_COUNT                                    // Total number of modes (for cycling)
};

// Animation Manager
// Coordinates ambient animations across all 4 channels
// Follows same pattern as NotificationManager
class AnimationManager {
public:
    AnimationManager(CRGB* ch1, CRGB* ch2, CRGB* ch3, CRGB* ch4, uint16_t numLeds) :
        channel1(ch1), channel2(ch2), channel3(ch3), channel4(ch4),
        numLedsPerChannel(numLeds),
        channelService1(nullptr), channelService2(nullptr),
        channelService3(nullptr), channelService4(nullptr),
        currentMode(AnimationMode::ANIM_NONE),
        lastUpdateMs(0),
        currentAnimation(nullptr) {
        // Load saved animation mode from NVS
        loadMode();
    }

    ~AnimationManager() {
        if (currentAnimation) {
            delete currentAnimation;
            currentAnimation = nullptr;
        }
    }

    // Set channel service pointers (call after channel services are created)
    void setChannelServices(DEV_LedChannel* ch1, DEV_LedChannel* ch2, DEV_LedChannel* ch3, DEV_LedChannel* ch4) {
        channelService1 = ch1;
        channelService2 = ch2;
        channelService3 = ch3;
        channelService4 = ch4;

        // Restore saved animation mode (if any)
        if (currentMode != AnimationMode::ANIM_NONE) {
            Serial.printf("Restoring saved animation: %s\n", getModeName(currentMode));
            AnimationMode savedMode = currentMode;
            currentMode = AnimationMode::ANIM_NONE;  // Reset to trigger proper initialization
            setMode(savedMode);
        }
    }

    // Toggle between normal and inverted animation
    // NONE <-> RAINBOW for modes without an inverted pair
    void toggleInverted() {
        constexpr int offset = static_cast<int>(AnimationMode::ANIM_BASE)
                             - static_cast<int>(AnimationMode::ANIM_INVERTED_BASE);
        int mode = static_cast<int>(currentMode);

        if (currentMode == AnimationMode::ANIM_NONE) {
            setMode(AnimationMode::ANIM_RAINBOW);
        } else if (currentMode == AnimationMode::ANIM_RAINBOW) {
            setMode(AnimationMode::ANIM_NONE);
        } else if (mode >= static_cast<int>(AnimationMode::ANIM_INVERTED_BASE)
                && mode <= static_cast<int>(AnimationMode::ANIM_INVERTED_SQUARE_TWINKLE)) {
            setMode(static_cast<AnimationMode>(mode + offset));
        } else {
            setMode(static_cast<AnimationMode>(mode - offset));
        }
    }

    // Cycle to next animation mode
    void cycleMode() {
        AnimationMode nextMode = static_cast<AnimationMode>(
            (static_cast<int>(currentMode) + 1) % static_cast<int>(AnimationMode::ANIM_COUNT));
        setMode(nextMode);
    }

    // Set specific animation mode
    void setMode(AnimationMode mode) {
        // Stop current animation if any
        if (currentMode != AnimationMode::ANIM_NONE) {
            stopCurrentAnimation();
        }

        currentMode = mode;

        // Save to NVS
        saveMode();

        // Start new animation
        if (currentMode != AnimationMode::ANIM_NONE) {
            startCurrentAnimation();
        }

        Serial.printf("Animation mode: %s\n", getModeName(currentMode));
    }

    // Update animation state (call from loop)
    void update() {
        if (!currentAnimation) {
            return;  // No animation active
        }

        unsigned long now = millis();
        unsigned long deltaMs = now - lastUpdateMs;
        lastUpdateMs = now;

        // Update current animation (polymorphic dispatch)
        bool needsRender = currentAnimation->update(deltaMs);

        // Render if needed
        if (needsRender) {
            renderCurrentAnimation();
        }
    }

    // Get current mode
    AnimationMode getCurrentMode() const {
        return currentMode;
    }

    // Check if animation is active
    bool isActive() const {
        return currentMode != AnimationMode::ANIM_NONE;
    }

    // Clear saved animation mode (for factory reset)
    void clearStorage() {
        Preferences prefs;
        if (prefs.begin("animation", false)) {
            prefs.clear();
            prefs.end();
            Serial.println("Animation mode storage cleared");
        }
    }

private:
    CRGB* channel1;
    CRGB* channel2;
    CRGB* channel3;
    CRGB* channel4;
    uint16_t numLedsPerChannel;

    DEV_LedChannel* channelService1;
    DEV_LedChannel* channelService2;
    DEV_LedChannel* channelService3;
    DEV_LedChannel* channelService4;

    AnimationMode currentMode;
    unsigned long lastUpdateMs;

    // Current animation (lazily instantiated, owned by this manager)
    AnimationBase* currentAnimation;

    // Factory: allocate a fresh animation for the given mode
    AnimationBase* createAnimation(AnimationMode mode) {
        switch (mode) {
            case AnimationMode::ANIM_MONOCHROMATIC_RUNNER:              return new MonochromaticRunner();
            case AnimationMode::ANIM_COMPLEMENTARY_RUNNER:              return new ComplementaryRunner();
            case AnimationMode::ANIM_SPLIT_COMPLEMENTARY_RUNNER:        return new SplitComplementaryRunner();
            case AnimationMode::ANIM_TRIADIC_RUNNER:                    return new TriadicRunner();
            case AnimationMode::ANIM_SQUARE_RUNNER:                     return new SquareRunner();
            case AnimationMode::ANIM_INVERTED_MONOCHROMATIC_RUNNER:     return new InvertedMonochromaticRunner();
            case AnimationMode::ANIM_INVERTED_COMPLEMENTARY_RUNNER:     return new InvertedComplementaryRunner();
            case AnimationMode::ANIM_INVERTED_SPLIT_COMPLEMENTARY_RUNNER: return new InvertedSplitComplementaryRunner();
            case AnimationMode::ANIM_INVERTED_TRIADIC_RUNNER:           return new InvertedTriadicRunner();
            case AnimationMode::ANIM_INVERTED_SQUARE_RUNNER:            return new InvertedSquareRunner();
            case AnimationMode::ANIM_MONOCHROMATIC_RAIN:                return new MonochromaticRain();
            case AnimationMode::ANIM_COMPLEMENTARY_RAIN:                return new ComplementaryRain();
            case AnimationMode::ANIM_SPLIT_COMPLEMENTARY_RAIN:          return new SplitComplementaryRain();
            case AnimationMode::ANIM_TRIADIC_RAIN:                      return new TriadicRain();
            case AnimationMode::ANIM_SQUARE_RAIN:                       return new SquareRain();
            case AnimationMode::ANIM_INVERTED_MONOCHROMATIC_RAIN:       return new InvertedMonochromaticRain();
            case AnimationMode::ANIM_INVERTED_COMPLEMENTARY_RAIN:       return new InvertedComplementaryRain();
            case AnimationMode::ANIM_INVERTED_SPLIT_COMPLEMENTARY_RAIN: return new InvertedSplitComplementaryRain();
            case AnimationMode::ANIM_INVERTED_TRIADIC_RAIN:             return new InvertedTriadicRain();
            case AnimationMode::ANIM_INVERTED_SQUARE_RAIN:              return new InvertedSquareRain();
            case AnimationMode::ANIM_MONOCHROMATIC_TWINKLE:             return new MonochromaticTwinkle();
            case AnimationMode::ANIM_COMPLEMENTARY_TWINKLE:             return new ComplementaryTwinkle();
            case AnimationMode::ANIM_SPLIT_COMPLEMENTARY_TWINKLE:       return new SplitComplementaryTwinkle();
            case AnimationMode::ANIM_TRIADIC_TWINKLE:                   return new TriadicTwinkle();
            case AnimationMode::ANIM_SQUARE_TWINKLE:                    return new SquareTwinkle();
            case AnimationMode::ANIM_INVERTED_MONOCHROMATIC_TWINKLE:    return new InvertedMonochromaticTwinkle();
            case AnimationMode::ANIM_INVERTED_COMPLEMENTARY_TWINKLE:    return new InvertedComplementaryTwinkle();
            case AnimationMode::ANIM_INVERTED_SPLIT_COMPLEMENTARY_TWINKLE: return new InvertedSplitComplementaryTwinkle();
            case AnimationMode::ANIM_INVERTED_TRIADIC_TWINKLE:          return new InvertedTriadicTwinkle();
            case AnimationMode::ANIM_INVERTED_SQUARE_TWINKLE:           return new InvertedSquareTwinkle();
            case AnimationMode::ANIM_BASE:                              return new BaseOnlyAnimation();
            case AnimationMode::ANIM_INVERTED_BASE:                     return new InvertedBaseAnimation();
            case AnimationMode::ANIM_RAINBOW:                           return new RainbowAnimation();
            default:                                                     return nullptr;
        }
    }

    // Storage for saved LED state (when entering animation mode)
    // RAM: 4ch × NUM_LEDS_PER_CHANNEL × 3 bytes/CRGB = ~2,400 bytes
    CRGB savedCh1[NUM_LEDS_PER_CHANNEL];
    CRGB savedCh2[NUM_LEDS_PER_CHANNEL];
    CRGB savedCh3[NUM_LEDS_PER_CHANNEL];
    CRGB savedCh4[NUM_LEDS_PER_CHANNEL];

    void startCurrentAnimation() {
        // Tell all channel services to yield to animation
        if (channelService1) channelService1->yieldToAnimation();
        if (channelService2) channelService2->yieldToAnimation();
        if (channelService3) channelService3->yieldToAnimation();
        if (channelService4) channelService4->yieldToAnimation();

        // Save current LED state
        for (int i = 0; i < numLedsPerChannel; i++) {
            savedCh1[i] = channel1[i];
            savedCh2[i] = channel2[i];
            savedCh3[i] = channel3[i];
            savedCh4[i] = channel4[i];
        }

        // Instantiate the new animation
        currentAnimation = createAnimation(currentMode);
        if (!currentAnimation) return;

        // Set channel hues and brightnesses from HomeKit state (polymorphic dispatch)
        if (channelService1 && channelService2 && channelService3 && channelService4) {
            currentAnimation->setChannelHues(
                channelService1->desired.hue,
                channelService2->desired.hue,
                channelService3->desired.hue,
                channelService4->desired.hue
            );
            currentAnimation->setChannelBrightnesses(
                channelService1->desired.brightness,
                channelService2->desired.brightness,
                channelService3->desired.brightness,
                channelService4->desired.brightness
            );
        }

        // Initialize animation (polymorphic dispatch)
        currentAnimation->begin();
        lastUpdateMs = millis();
    }

    void stopCurrentAnimation() {
        // Delete the current animation instance (lazy instantiation)
        delete currentAnimation;
        currentAnimation = nullptr;

        // Restore saved LED state
        for (int i = 0; i < numLedsPerChannel; i++) {
            channel1[i] = savedCh1[i];
            channel2[i] = savedCh2[i];
            channel3[i] = savedCh3[i];
            channel4[i] = savedCh4[i];
        }

        // Tell all channel services to resume from animation
        if (channelService1) channelService1->resumeFromAnimation();
        if (channelService2) channelService2->resumeFromAnimation();
        if (channelService3) channelService3->resumeFromAnimation();
        if (channelService4) channelService4->resumeFromAnimation();
    }

    void renderCurrentAnimation() {
        if (!currentAnimation) return;

        // Update animation hues and brightnesses from current HomeKit state (polymorphic dispatch)
        if (channelService1 && channelService2 && channelService3 && channelService4) {
            currentAnimation->setChannelHues(
                channelService1->desired.hue,
                channelService2->desired.hue,
                channelService3->desired.hue,
                channelService4->desired.hue
            );
            currentAnimation->setChannelBrightnesses(
                channelService1->desired.brightness,
                channelService2->desired.brightness,
                channelService3->desired.brightness,
                channelService4->desired.brightness
            );
        }

        // Render animation (polymorphic dispatch)
        currentAnimation->render(channel1, channel2, channel3, channel4, numLedsPerChannel);

        // Respect HomeKit power state: turn off channels that are OFF
        if (channelService1 && !channelService1->desired.power) {
            fill_solid(channel1, numLedsPerChannel, CRGB::Black);
        }
        if (channelService2 && !channelService2->desired.power) {
            fill_solid(channel2, numLedsPerChannel, CRGB::Black);
        }
        if (channelService3 && !channelService3->desired.power) {
            fill_solid(channel3, numLedsPerChannel, CRGB::Black);
        }
        if (channelService4 && !channelService4->desired.power) {
            fill_solid(channel4, numLedsPerChannel, CRGB::Black);
        }
    }

    const char* getModeName(AnimationMode mode) const {
        switch (mode) {
            case AnimationMode::ANIM_NONE:                              return "HomeKit";
            case AnimationMode::ANIM_INVERTED_BASE:                     return "Inverted Base";
            case AnimationMode::ANIM_BASE:                              return "Base";
            case AnimationMode::ANIM_INVERTED_MONOCHROMATIC_RUNNER:     return "Inv. Monochromatic Runner";
            case AnimationMode::ANIM_MONOCHROMATIC_RUNNER:              return "Monochromatic Runner";
            case AnimationMode::ANIM_INVERTED_COMPLEMENTARY_RUNNER:     return "Inv. Complementary Runner";
            case AnimationMode::ANIM_COMPLEMENTARY_RUNNER:              return "Complementary Runner";
            case AnimationMode::ANIM_INVERTED_SPLIT_COMPLEMENTARY_RUNNER: return "Inv. Split-Comp Runner";
            case AnimationMode::ANIM_SPLIT_COMPLEMENTARY_RUNNER:        return "Split-Complementary Runner";
            case AnimationMode::ANIM_INVERTED_TRIADIC_RUNNER:           return "Inv. Triadic Runner";
            case AnimationMode::ANIM_TRIADIC_RUNNER:                    return "Triadic Runner";
            case AnimationMode::ANIM_INVERTED_SQUARE_RUNNER:            return "Inv. Square Runner";
            case AnimationMode::ANIM_SQUARE_RUNNER:                     return "Square Runner";
            case AnimationMode::ANIM_INVERTED_MONOCHROMATIC_RAIN:       return "Inv. Monochromatic Rain";
            case AnimationMode::ANIM_MONOCHROMATIC_RAIN:                return "Monochromatic Rain";
            case AnimationMode::ANIM_INVERTED_COMPLEMENTARY_RAIN:       return "Inv. Complementary Rain";
            case AnimationMode::ANIM_COMPLEMENTARY_RAIN:                return "Complementary Rain";
            case AnimationMode::ANIM_INVERTED_SPLIT_COMPLEMENTARY_RAIN: return "Inv. Split-Comp Rain";
            case AnimationMode::ANIM_SPLIT_COMPLEMENTARY_RAIN:          return "Split-Complementary Rain";
            case AnimationMode::ANIM_INVERTED_TRIADIC_RAIN:             return "Inv. Triadic Rain";
            case AnimationMode::ANIM_TRIADIC_RAIN:                      return "Triadic Rain";
            case AnimationMode::ANIM_INVERTED_SQUARE_RAIN:              return "Inv. Square Rain";
            case AnimationMode::ANIM_SQUARE_RAIN:                       return "Square Rain";
            case AnimationMode::ANIM_INVERTED_MONOCHROMATIC_TWINKLE:    return "Inv. Monochromatic Twinkle";
            case AnimationMode::ANIM_MONOCHROMATIC_TWINKLE:             return "Monochromatic Twinkle";
            case AnimationMode::ANIM_INVERTED_COMPLEMENTARY_TWINKLE:    return "Inv. Complementary Twinkle";
            case AnimationMode::ANIM_COMPLEMENTARY_TWINKLE:             return "Complementary Twinkle";
            case AnimationMode::ANIM_INVERTED_SPLIT_COMPLEMENTARY_TWINKLE: return "Inv. Split-Comp Twinkle";
            case AnimationMode::ANIM_SPLIT_COMPLEMENTARY_TWINKLE:       return "Split-Complementary Twinkle";
            case AnimationMode::ANIM_INVERTED_TRIADIC_TWINKLE:          return "Inv. Triadic Twinkle";
            case AnimationMode::ANIM_TRIADIC_TWINKLE:                   return "Triadic Twinkle";
            case AnimationMode::ANIM_INVERTED_SQUARE_TWINKLE:           return "Inv. Square Twinkle";
            case AnimationMode::ANIM_SQUARE_TWINKLE:                    return "Square Twinkle";
            case AnimationMode::ANIM_RAINBOW:                           return "Rainbow";
            default:                                                     return "Unknown";
        }
    }

    // Load animation mode from NVS
    void loadMode() {
        Preferences prefs;
        if (!prefs.begin("animation", true)) {  // true = read-only
            return;
        }

        if (prefs.isKey("mode")) {
            uint8_t savedMode = prefs.getUChar("mode", 0);
            if (savedMode < static_cast<uint8_t>(AnimationMode::ANIM_COUNT)) {
                currentMode = static_cast<AnimationMode>(savedMode);
                Serial.printf("Loaded animation mode from NVS: %s\n", getModeName(currentMode));

                // Start the saved animation
                if (currentMode != AnimationMode::ANIM_NONE) {
                    // Note: This will be called in setup() after channel services are configured
                    // The actual animation start happens when setChannelServices() is called
                }
            }
        }

        prefs.end();
    }

    // Save animation mode to NVS
    void saveMode() {
        Preferences prefs;
        if (!prefs.begin("animation", false)) {  // false = read-write
            Serial.println("Failed to open NVS namespace: animation");
            return;
        }

        prefs.putUChar("mode", static_cast<uint8_t>(currentMode));
        prefs.end();

        Serial.printf("Saved animation mode to NVS: %s\n", getModeName(currentMode));
    }
};
