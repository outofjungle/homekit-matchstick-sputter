#pragma once

#include <Preferences.h>
#include "animation_base.h"
#include "monochromatic_runner.h"
#include "complementary_runner.h"
#include "split_complementary_runner.h"
#include "triadic_runner.h"
#include "square_runner.h"
#include "monochromatic_twinkle.h"
#include "complementary_twinkle.h"
#include "split_complementary_twinkle.h"
#include "triadic_twinkle.h"
#include "square_twinkle.h"
#include "../led_channel.h"

// Animation modes
enum AnimationMode {
    ANIM_NONE,                  // HomeKit control (normal operation)
    ANIM_MONOCHROMATIC_RUNNER,  // Monochromatic runner (black/white)
    ANIM_COMPLEMENTARY_RUNNER,  // Complementary runner (2 colors)
    ANIM_SPLIT_COMPLEMENTARY_RUNNER, // Split-complementary runner (3 colors)
    ANIM_TRIADIC_RUNNER,        // Triadic runner (3 colors)
    ANIM_SQUARE_RUNNER,         // Square runner (4 colors)
    ANIM_MONOCHROMATIC,         // Monochromatic twinkle (was ANIM_TWINKLE)
    ANIM_COMPLEMENTARY,         // Complementary twinkle (2 colors)
    ANIM_SPLIT_COMPLEMENTARY,   // Split-complementary twinkle (3 colors)
    ANIM_TRIADIC,               // Triadic twinkle (3 colors)
    ANIM_SQUARE,                // Square twinkle (4 colors)
    ANIM_COUNT                  // Total number of modes (for cycling)
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
        currentMode(ANIM_NONE),
        lastUpdateMs(0) {
        // Load saved animation mode from NVS
        loadMode();
    }

    // Set channel service pointers (call after channel services are created)
    void setChannelServices(DEV_LedChannel* ch1, DEV_LedChannel* ch2, DEV_LedChannel* ch3, DEV_LedChannel* ch4) {
        channelService1 = ch1;
        channelService2 = ch2;
        channelService3 = ch3;
        channelService4 = ch4;

        // Restore saved animation mode (if any)
        if (currentMode != ANIM_NONE) {
            Serial.printf("Restoring saved animation: %s\n", getModeName(currentMode));
            AnimationMode savedMode = currentMode;
            currentMode = ANIM_NONE;  // Reset to trigger proper initialization
            setMode(savedMode);
        }
    }

    // Cycle to next animation mode
    void cycleMode() {
        AnimationMode nextMode = (AnimationMode)((currentMode + 1) % ANIM_COUNT);
        setMode(nextMode);
    }

    // Set specific animation mode
    void setMode(AnimationMode mode) {
        // Stop current animation if any
        if (currentMode != ANIM_NONE) {
            stopCurrentAnimation();
        }

        currentMode = mode;

        // Save to NVS
        saveMode();

        // Start new animation
        if (currentMode != ANIM_NONE) {
            startCurrentAnimation();
        }

        Serial.printf("Animation mode: %s\n", getModeName(currentMode));
    }

    // Update animation state (call from loop)
    void update() {
        if (currentMode == ANIM_NONE) {
            return;  // No animation active
        }

        unsigned long now = millis();
        unsigned long deltaMs = now - lastUpdateMs;
        lastUpdateMs = now;

        bool needsRender = false;

        // Update current animation
        switch (currentMode) {
            case ANIM_MONOCHROMATIC_RUNNER:
                needsRender = monochromaticRunnerAnim.update(deltaMs);
                break;

            case ANIM_COMPLEMENTARY_RUNNER:
                needsRender = complementaryRunnerAnim.update(deltaMs);
                break;

            case ANIM_SPLIT_COMPLEMENTARY_RUNNER:
                needsRender = splitComplementaryRunnerAnim.update(deltaMs);
                break;

            case ANIM_TRIADIC_RUNNER:
                needsRender = triadicRunnerAnim.update(deltaMs);
                break;

            case ANIM_SQUARE_RUNNER:
                needsRender = squareRunnerAnim.update(deltaMs);
                break;

            case ANIM_MONOCHROMATIC:
                needsRender = monochromaticAnim.update(deltaMs);
                break;

            case ANIM_COMPLEMENTARY:
                needsRender = complementaryAnim.update(deltaMs);
                break;

            case ANIM_SPLIT_COMPLEMENTARY:
                needsRender = splitComplementaryAnim.update(deltaMs);
                break;

            case ANIM_TRIADIC:
                needsRender = triadicAnim.update(deltaMs);
                break;

            case ANIM_SQUARE:
                needsRender = squareAnim.update(deltaMs);
                break;

            default:
                break;
        }

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
        return currentMode != ANIM_NONE;
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

    // Animation instances
    MonochromaticRunner monochromaticRunnerAnim;
    ComplementaryRunner complementaryRunnerAnim;
    SplitComplementaryRunner splitComplementaryRunnerAnim;
    TriadicRunner triadicRunnerAnim;
    SquareRunner squareRunnerAnim;
    MonochromaticTwinkle monochromaticAnim;
    ComplementaryTwinkle complementaryAnim;
    SplitComplementaryTwinkle splitComplementaryAnim;
    TriadicTwinkle triadicAnim;
    SquareTwinkle squareAnim;

    // Storage for saved LED state (when entering animation mode)
    CRGB savedCh1[200];
    CRGB savedCh2[200];
    CRGB savedCh3[200];
    CRGB savedCh4[200];

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

        // Initialize animation
        switch (currentMode) {
            case ANIM_MONOCHROMATIC_RUNNER:
                // Set channel hues and brightnesses from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    monochromaticRunnerAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                    monochromaticRunnerAnim.setChannelBrightnesses(
                        channelService1->desired.brightness,
                        channelService2->desired.brightness,
                        channelService3->desired.brightness,
                        channelService4->desired.brightness
                    );
                }
                monochromaticRunnerAnim.begin();
                break;

            case ANIM_COMPLEMENTARY_RUNNER:
                // Set channel hues and brightnesses from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    complementaryRunnerAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                    complementaryRunnerAnim.setChannelBrightnesses(
                        channelService1->desired.brightness,
                        channelService2->desired.brightness,
                        channelService3->desired.brightness,
                        channelService4->desired.brightness
                    );
                }
                complementaryRunnerAnim.begin();
                break;

            case ANIM_SPLIT_COMPLEMENTARY_RUNNER:
                // Set channel hues and brightnesses from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    splitComplementaryRunnerAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                    splitComplementaryRunnerAnim.setChannelBrightnesses(
                        channelService1->desired.brightness,
                        channelService2->desired.brightness,
                        channelService3->desired.brightness,
                        channelService4->desired.brightness
                    );
                }
                splitComplementaryRunnerAnim.begin();
                break;

            case ANIM_TRIADIC_RUNNER:
                // Set channel hues and brightnesses from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    triadicRunnerAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                    triadicRunnerAnim.setChannelBrightnesses(
                        channelService1->desired.brightness,
                        channelService2->desired.brightness,
                        channelService3->desired.brightness,
                        channelService4->desired.brightness
                    );
                }
                triadicRunnerAnim.begin();
                break;

            case ANIM_SQUARE_RUNNER:
                // Set channel hues and brightnesses from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    squareRunnerAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                    squareRunnerAnim.setChannelBrightnesses(
                        channelService1->desired.brightness,
                        channelService2->desired.brightness,
                        channelService3->desired.brightness,
                        channelService4->desired.brightness
                    );
                }
                squareRunnerAnim.begin();
                break;

            case ANIM_MONOCHROMATIC:
                // Set channel hues from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    monochromaticAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                }
                monochromaticAnim.begin();
                break;

            case ANIM_COMPLEMENTARY:
                // Set channel hues from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    complementaryAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                }
                complementaryAnim.begin();
                break;

            case ANIM_SPLIT_COMPLEMENTARY:
                // Set channel hues from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    splitComplementaryAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                }
                splitComplementaryAnim.begin();
                break;

            case ANIM_TRIADIC:
                // Set channel hues from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    triadicAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                }
                triadicAnim.begin();
                break;

            case ANIM_SQUARE:
                // Set channel hues from HomeKit state
                if (channelService1 && channelService2 && channelService3 && channelService4) {
                    squareAnim.setChannelHues(
                        channelService1->desired.hue,
                        channelService2->desired.hue,
                        channelService3->desired.hue,
                        channelService4->desired.hue
                    );
                }
                squareAnim.begin();
                break;

            default:
                break;
        }

        lastUpdateMs = millis();
    }

    void stopCurrentAnimation() {
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
        // Update animation hues and brightnesses from current HomeKit state (allows real-time changes)
        if (channelService1 && channelService2 && channelService3 && channelService4) {
            int h1 = channelService1->desired.hue;
            int h2 = channelService2->desired.hue;
            int h3 = channelService3->desired.hue;
            int h4 = channelService4->desired.hue;

            int b1 = channelService1->desired.brightness;
            int b2 = channelService2->desired.brightness;
            int b3 = channelService3->desired.brightness;
            int b4 = channelService4->desired.brightness;

            switch (currentMode) {
                case ANIM_MONOCHROMATIC_RUNNER:
                    monochromaticRunnerAnim.setChannelHues(h1, h2, h3, h4);
                    monochromaticRunnerAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_COMPLEMENTARY_RUNNER:
                    complementaryRunnerAnim.setChannelHues(h1, h2, h3, h4);
                    complementaryRunnerAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_SPLIT_COMPLEMENTARY_RUNNER:
                    splitComplementaryRunnerAnim.setChannelHues(h1, h2, h3, h4);
                    splitComplementaryRunnerAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_TRIADIC_RUNNER:
                    triadicRunnerAnim.setChannelHues(h1, h2, h3, h4);
                    triadicRunnerAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_SQUARE_RUNNER:
                    squareRunnerAnim.setChannelHues(h1, h2, h3, h4);
                    squareRunnerAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_MONOCHROMATIC:
                    monochromaticAnim.setChannelHues(h1, h2, h3, h4);
                    break;
                case ANIM_COMPLEMENTARY:
                    complementaryAnim.setChannelHues(h1, h2, h3, h4);
                    complementaryAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_SPLIT_COMPLEMENTARY:
                    splitComplementaryAnim.setChannelHues(h1, h2, h3, h4);
                    splitComplementaryAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_TRIADIC:
                    triadicAnim.setChannelHues(h1, h2, h3, h4);
                    triadicAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                case ANIM_SQUARE:
                    squareAnim.setChannelHues(h1, h2, h3, h4);
                    squareAnim.setChannelBrightnesses(b1, b2, b3, b4);
                    break;
                default:
                    break;
            }
        }

        // Render animation
        switch (currentMode) {
            case ANIM_MONOCHROMATIC_RUNNER:
                monochromaticRunnerAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_COMPLEMENTARY_RUNNER:
                complementaryRunnerAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_SPLIT_COMPLEMENTARY_RUNNER:
                splitComplementaryRunnerAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_TRIADIC_RUNNER:
                triadicRunnerAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_SQUARE_RUNNER:
                squareRunnerAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_MONOCHROMATIC:
                monochromaticAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_COMPLEMENTARY:
                complementaryAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_SPLIT_COMPLEMENTARY:
                splitComplementaryAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_TRIADIC:
                triadicAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            case ANIM_SQUARE:
                squareAnim.render(channel1, channel2, channel3, channel4, numLedsPerChannel);
                break;

            default:
                break;
        }

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
            case ANIM_NONE: return "HomeKit";
            case ANIM_MONOCHROMATIC_RUNNER: return "Monochromatic Runner";
            case ANIM_COMPLEMENTARY_RUNNER: return "Complementary Runner";
            case ANIM_SPLIT_COMPLEMENTARY_RUNNER: return "Split-Complementary Runner";
            case ANIM_TRIADIC_RUNNER: return "Triadic Runner";
            case ANIM_SQUARE_RUNNER: return "Square Runner";
            case ANIM_MONOCHROMATIC: return "Monochromatic Twinkle";
            case ANIM_COMPLEMENTARY: return "Complementary Twinkle";
            case ANIM_SPLIT_COMPLEMENTARY: return "Split-Complementary Twinkle";
            case ANIM_TRIADIC: return "Triadic Twinkle";
            case ANIM_SQUARE: return "Square Twinkle";
            default: return "Unknown";
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
            if (savedMode < ANIM_COUNT) {
                currentMode = (AnimationMode)savedMode;
                Serial.printf("Loaded animation mode from NVS: %s\n", getModeName(currentMode));

                // Start the saved animation
                if (currentMode != ANIM_NONE) {
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

        prefs.putUChar("mode", (uint8_t)currentMode);
        prefs.end();

        Serial.printf("Saved animation mode to NVS: %s\n", getModeName(currentMode));
    }
};
