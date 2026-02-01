#pragma once

#include "HomeSpan.h"
#include <FastLED.h>
#include "channel_storage.h"

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

    // Boot flash state (for 5-second flash when brightness=0)
    bool bootFlashActive = false;
    unsigned long bootFlashStartMs = 0;
    int bootFlashHue = 0;        // Store hue for boot flash display
    int bootFlashSat = 100;      // Store saturation for boot flash display
    static constexpr unsigned long BOOT_FLASH_DURATION_MS = 5000;
    static constexpr int BOOT_FLASH_BRIGHTNESS = 25;  // Visible brightness during flash

    // Default hue values per channel (0°, 90°, 270°, 180°)
    static int getDefaultHue(int channelNum) {
        switch (channelNum) {
            case 1: return 0;    // Red
            case 2: return 90;   // Yellow-green
            case 3: return 270;  // Purple/violet
            case 4: return 180;  // Cyan
            default: return 0;
        }
    }

    // Constructor - initializes the LightBulb service with HSV characteristics
    DEV_LedChannel(CRGB* ledArray, uint16_t count, int channelNum)
        : Service::LightBulb(), storage(channelNum) {
        leds = ledArray;
        numLeds = count;
        channelNumber = channelNum;

        // Try to load saved state from NVS
        ChannelStorage::ChannelState savedState;
        bool hasStoredState = storage.load(savedState);

        if (hasStoredState) {
            // Restore saved values
            power = new Characteristic::On(savedState.power);
            hue = new Characteristic::Hue(savedState.hue);
            saturation = new Characteristic::Saturation(savedState.saturation);
            brightness = new Characteristic::Brightness(savedState.brightness);

            Serial.printf("Channel %d: Loaded state from NVS - Power=%s H=%d S=%d%% B=%d%%\n",
                         channelNum,
                         savedState.power ? "ON" : "OFF",
                         savedState.hue,
                         savedState.saturation,
                         savedState.brightness);

            // If brightness is 0, activate boot flash with visible brightness
            if (savedState.brightness == 0) {
                bootFlashActive = true;
                bootFlashStartMs = millis();
                bootFlashHue = savedState.hue;
                bootFlashSat = savedState.saturation;
                // Show the color at visible brightness
                applyLedState(true, savedState.hue, savedState.saturation, BOOT_FLASH_BRIGHTNESS);
                Serial.printf("Channel %d: Boot flash activated (brightness=0), showing at %d%%\n",
                             channelNum, BOOT_FLASH_BRIGHTNESS);
            } else {
                // Apply the saved state normally
                applyLedState(savedState.power, savedState.hue, savedState.saturation, savedState.brightness);
            }
        } else {
            // No saved state - use channel-specific defaults
            int defaultHue = getDefaultHue(channelNum);
            power = new Characteristic::On(1);                         // Default: On
            hue = new Characteristic::Hue(defaultHue);                 // Channel-specific hue
            saturation = new Characteristic::Saturation(100);          // Default: Full saturation
            brightness = new Characteristic::Brightness(25);           // Default: 25% brightness

            Serial.printf("Channel %d: No saved state, using defaults (H=%d, B=25%%)\n",
                         channelNum, defaultHue);

            // Apply the default state to LEDs
            applyLedState(true, defaultHue, 100, 25);

            // Save the defaults to NVS
            ChannelStorage::ChannelState defaultState;
            defaultState.power = true;
            defaultState.hue = defaultHue;
            defaultState.saturation = 100;
            defaultState.brightness = 25;
            storage.save(defaultState);
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

    // Update handler - called when HomeKit sends new values
    boolean update() override {
        bool powerOn = power->getNewVal();
        int h = hue->getNewVal();
        int s = saturation->getNewVal();
        int v = brightness->getNewVal();

        // Apply the LED state
        applyLedState(powerOn, h, s, v);

        // Debug output
        if (powerOn) {
            Serial.printf("Channel %d updated: H=%d S=%d%% V=%d%% (Power: ON)\n",
                         channelNumber, h, s, v);
        } else {
            Serial.printf("Channel %d updated: Power OFF\n", channelNumber);
        }

        // Save state to NVS
        ChannelStorage::ChannelState state;
        state.power = powerOn;
        state.hue = h;
        state.saturation = s;
        state.brightness = v;
        storage.save(state);

        // Cancel boot flash if active (user has changed the state)
        bootFlashActive = false;

        return true;  // Return true to indicate successful update
    }

    // Call this in loop() to handle boot flash timeout
    void loop() {
        if (bootFlashActive) {
            unsigned long elapsed = millis() - bootFlashStartMs;
            if (elapsed >= BOOT_FLASH_DURATION_MS) {
                // Flash duration expired - turn off LEDs (restore brightness=0 state)
                Serial.printf("Channel %d: Boot flash expired, turning off\n", channelNumber);
                fill_solid(leds, numLeds, CRGB::Black);
                bootFlashActive = false;
            }
        }
    }

    // Check if boot flash is currently active
    bool isBootFlashActive() {
        return bootFlashActive;
    }
};
