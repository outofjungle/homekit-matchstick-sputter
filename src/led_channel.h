#pragma once

#include "HomeSpan.h"
#include <FastLED.h>

// HomeKit LightBulb service for controlling an LED channel
struct DEV_LedChannel : Service::LightBulb {
    CRGB* leds;                          // Pointer to LED array for this channel
    uint16_t numLeds;                    // Number of LEDs in this channel

    SpanCharacteristic *power;           // On/Off characteristic
    SpanCharacteristic *hue;             // Hue (0-360 degrees)
    SpanCharacteristic *saturation;      // Saturation (0-100%)
    SpanCharacteristic *brightness;      // Brightness (0-100%)

    // Constructor - initializes the LightBulb service with HSV characteristics
    DEV_LedChannel(CRGB* ledArray, uint16_t count) : Service::LightBulb() {
        leds = ledArray;
        numLeds = count;

        // Create HomeKit characteristics with default values
        power = new Characteristic::On(0);                         // Default: Off
        hue = new Characteristic::Hue(0);                          // Default: Red (0°)
        saturation = new Characteristic::Saturation(100);          // Default: Full saturation
        brightness = new Characteristic::Brightness(100);          // Default: Full brightness
    }

    // Update handler - called when HomeKit sends new values
    boolean update() override {
        if (power->getNewVal()) {
            // Light is ON - convert HomeKit HSV to FastLED CHSV
            // HomeKit: H=0-360, S=0-100, V=0-100
            // FastLED CHSV: H=0-255, S=0-255, V=0-255
            uint8_t h = map(hue->getNewVal(), 0, 360, 0, 255);
            uint8_t s = map(saturation->getNewVal(), 0, 100, 0, 255);
            uint8_t v = map(brightness->getNewVal(), 0, 100, 0, 255);

            // Fill all LEDs with the color (FastLED handles HSV→RGB conversion)
            fill_solid(leds, numLeds, CHSV(h, s, v));

            // Debug output
            Serial.printf("Channel updated: H=%d S=%d%% V=%d%% (Power: ON)\n",
                         hue->getNewVal(),
                         saturation->getNewVal(),
                         brightness->getNewVal());
        } else {
            // Light is OFF - turn off all LEDs
            fill_solid(leds, numLeds, CRGB::Black);
            Serial.println("Channel updated: Power OFF");
        }

        return true;  // Return true to indicate successful update
    }
};
