#pragma once

#include "HomeSpan.h"
#include <FastLED.h>
#include "channel_storage.h"
#include "config.h"

// LED Channel State Machine
enum class ChannelState {
    NORMAL,         // Normal HomeKit-controlled operation
    NOTIFICATION,   // Yielded to notification system (highest priority)
    ANIMATION,      // Ambient animation active
    OFF             // Power is off
};

// HomeKit LightBulb service for controlling an LED channel
struct DEV_LedChannel : Service::LightBulb {
    CRGB* leds;                          // Pointer to LED array for this channel
    uint16_t numLeds;                    // Number of LEDs in this channel
    ChannelStorage storage;              // NVS storage for this channel
    int channelNumber;                   // Channel identifier (1-4)

    SpanCharacteristic *power;           // On/Off characteristic
    SpanCharacteristic *hue;             // Hue (0-360 degrees)
    SpanCharacteristic *saturation;      // Saturation (0-100%)
    SpanCharacteristic *brightness;      // Brightness (0-100%)

    // FSM state
    ChannelState currentState = ChannelState::NORMAL;
    unsigned long stateEnteredMs = 0;

    // Desired state (what we want to show when not in NOTIFICATION/BOOT_FLASH)
    struct {
        bool power;
        int hue;
        int saturation;
        int brightness;
    } desired;

    // Constructor - initializes the LightBulb service with HSV characteristics
    DEV_LedChannel(CRGB* ledArray, uint16_t count, int channelNum)
        : Service::LightBulb(), storage(channelNum) {
        leds = ledArray;
        numLeds = count;
        channelNumber = channelNum;

        // Load validated state from NVS (guaranteed valid by applyChannelDefaults)
        ChannelStorage::ChannelState savedState;
        storage.load(savedState);

        // Create HomeKit characteristics with saved values
        power = new Characteristic::On(savedState.power);
        hue = new Characteristic::Hue(savedState.hue);
        saturation = new Characteristic::Saturation(savedState.saturation);
        brightness = new Characteristic::Brightness(savedState.brightness);

        // Store desired state
        desired.power = savedState.power;
        desired.hue = savedState.hue;
        desired.saturation = savedState.saturation;
        desired.brightness = savedState.brightness;

        Serial.printf("Channel %d: Loaded - Power=%s H=%d S=%d%% B=%d%%\n",
                     channelNum,
                     savedState.power ? "ON" : "OFF",
                     savedState.hue, savedState.saturation, savedState.brightness);

        // Enter FSM state (power guaranteed ON by defaults, but handle anyway)
        if (!savedState.power) {
            enterState(ChannelState::OFF);
        } else {
            enterState(ChannelState::NORMAL);
        }
    }

    // Helper method to apply LED state
    void applyLedState(bool powerOn, int h, int s, int v) {
        if (powerOn) {
            // Light is ON - convert HomeKit HSV to FastLED CHSV
            // HomeKit: H=0-360, S=0-100, V=0-100
            // FastLED CHSV: H=0-255, S=0-255, V=0-255
            uint8_t h_8 = map(h, 0, 360, 0, 255);
            uint8_t s_8 = map(s, 0, 100, 0, 255);
            uint8_t v_8 = map(v, 0, 100, 0, 255);

            // Fill all LEDs with the color (FastLED handles HSV→RGB conversion)
            fill_solid(leds, numLeds, CHSV(h_8, s_8, v_8));
        } else {
            // Light is OFF - turn off all LEDs
            fill_solid(leds, numLeds, CRGB::Black);
        }
    }

    // FSM: Enter a new state
    void enterState(ChannelState newState) {
        currentState = newState;
        stateEnteredMs = millis();

        // Render based on new state
        switch (newState) {
            case ChannelState::NORMAL:
                // Show desired state
                applyLedState(desired.power, desired.hue, desired.saturation, desired.brightness);
                break;

            case ChannelState::OFF:
                // Turn off
                applyLedState(false, 0, 0, 0);
                break;

            case ChannelState::NOTIFICATION:
                // Notification system will handle rendering
                break;

            case ChannelState::ANIMATION:
                // Animation system will handle rendering
                break;
        }
    }

    // FSM: Time-based state transitions
    void updateFSM() {
        // No time-based transitions needed (BOOT_FLASH removed)
    }

    // FSM: Yield control to notification system
    void yieldToNotification() {
        if (currentState != ChannelState::NOTIFICATION) {
            enterState(ChannelState::NOTIFICATION);
        }
    }

    // FSM: Resume from notification
    void resumeFromNotification() {
        if (currentState == ChannelState::NOTIFICATION) {
            // Return to appropriate state based on desired values
            if (!desired.power) {
                enterState(ChannelState::OFF);
            } else {
                enterState(ChannelState::NORMAL);
            }
        }
    }

    // FSM: Yield control to animation system
    void yieldToAnimation() {
        if (currentState != ChannelState::NOTIFICATION && currentState != ChannelState::ANIMATION) {
            enterState(ChannelState::ANIMATION);
        }
    }

    // FSM: Resume from animation
    void resumeFromAnimation() {
        if (currentState == ChannelState::ANIMATION) {
            // Return to appropriate state based on desired values
            if (!desired.power) {
                enterState(ChannelState::OFF);
            } else {
                enterState(ChannelState::NORMAL);
            }
        }
    }

    // Update handler - called when HomeKit sends new values
    boolean update() override {
        bool powerOn = power->getNewVal();
        int h = hue->getNewVal();
        int s = saturation->getNewVal();
        int v = brightness->getNewVal();

        // Force brightness=0 to DEFAULT_BRIGHTNESS
        int clampedBrightness = (v == 0) ? DEFAULT_BRIGHTNESS : v;

        // Update desired state
        desired.power = powerOn;
        desired.hue = h;
        desired.saturation = s;
        desired.brightness = clampedBrightness;

        // Debug output
        if (powerOn) {
            Serial.printf("Channel %d updated: H=%d S=%d%% V=%d%%", channelNumber, h, s, clampedBrightness);
            if (v == 0) {
                Serial.printf(" (forced from 0)");
            }
            Serial.printf(" (Power: ON)\n");
        } else {
            Serial.printf("Channel %d updated: Power OFF\n", channelNumber);
        }

        // Save state to NVS
        ChannelStorage::ChannelState state;
        state.power = powerOn;
        state.hue = h;
        state.saturation = s;
        state.brightness = clampedBrightness;
        storage.save(state);

        // Transition to appropriate state
        if (currentState == ChannelState::NORMAL || currentState == ChannelState::OFF) {
            if (!powerOn) {
                enterState(ChannelState::OFF);
            } else {
                enterState(ChannelState::NORMAL);
            }
        }

        return true;  // Return true to indicate successful update
    }

    // Clear this channel's NVS storage (used during factory reset)
    void clearStorage() {
        storage.clear();
        Serial.printf("Channel %d: Storage cleared\n", channelNumber);
    }
};
