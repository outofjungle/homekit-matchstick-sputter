#include <Arduino.h>
#include <FastLED.h>
#include "HomeSpan.h"
#include "config.h"
#include "led_channel.h"
#include "wifi_credentials.h"

// LED Arrays for all channels
CRGB ledChannel1[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 26
CRGB ledChannel2[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 18
CRGB ledChannel3[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 25
CRGB ledChannel4[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 19

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n========================================");
    Serial.println("homekit-matchstick-sputter - Phase 2");
    Serial.println("HomeKit Integration - 4 Light Channels");
    Serial.println("========================================");

    // Initialize FastLED for all channels
    FastLED.addLeds<WS2811, PIN_LED_CH1, GRB>(ledChannel1, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH2, GRB>(ledChannel2, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH3, GRB>(ledChannel3, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH4, GRB>(ledChannel4, NUM_LEDS_PER_CHANNEL);

    // Set brightness (25% for safe testing)
    FastLED.setBrightness(64);

    // Initialize all LEDs to off
    fill_solid(ledChannel1, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel2, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel3, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel4, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    FastLED.show();

    Serial.println("FastLED initialized.");

    // Set WiFi credentials before HomeSpan initialization
    homeSpan.setWifiCredentials(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("WiFi credentials configured.");

    // Initialize HomeSpan
    homeSpan.begin(Category::Bridges, DEVICE_NAME);

    Serial.println("HomeSpan initialized.");
    Serial.println("Creating HomeKit accessories...");

    // Create Bridge Accessory (required first)
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name(DEVICE_NAME);
            new Characteristic::Manufacturer(DEVICE_MANUFACTURER);
            new Characteristic::SerialNumber(DEVICE_SERIAL);
            new Characteristic::Model(DEVICE_MODEL);
            new Characteristic::FirmwareRevision(DEVICE_FIRMWARE);

    // Create Channel 1 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 1");
        new DEV_LedChannel(ledChannel1, NUM_LEDS_PER_CHANNEL);

    // Create Channel 2 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 2");
        new DEV_LedChannel(ledChannel2, NUM_LEDS_PER_CHANNEL);

    // Create Channel 3 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 3");
        new DEV_LedChannel(ledChannel3, NUM_LEDS_PER_CHANNEL);

    // Create Channel 4 Accessory
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
            new Characteristic::Name("Channel 4");
        new DEV_LedChannel(ledChannel4, NUM_LEDS_PER_CHANNEL);

    Serial.println("========================================");
    Serial.println("Setup complete!");
    Serial.println("Press 'W' in serial monitor to configure WiFi");
    Serial.println("After WiFi is connected, pair with HomeKit");
    Serial.println("========================================\n");
}

void loop() {
    // Poll HomeSpan for HomeKit events
    homeSpan.poll();

    // Update LED strips (called after every HomeSpan update)
    FastLED.show();
}
