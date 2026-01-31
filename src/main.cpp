#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

// LED Arrays
CRGB ledChannel0[1];                           // SK6812 RGB on GPIO 27
CRGB ledChannel1[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 26
CRGB ledChannel2[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 18
CRGB ledChannel3[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 25
CRGB ledChannel4[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 19

// Blink state
unsigned long lastBlinkTime = 0;
bool blinkState = false;

// Button state
unsigned long lastButtonPress = 0;
bool lastButtonState = HIGH;  // GPIO39 has pullup
uint8_t currentColorIndex = 0;  // For cycling channel 0 colors
bool statusLedState = false;    // For toggling status LED

// Color palette for cycling
const CRGB colors[4] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White};

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n========================================");
    Serial.println("homekit-matchstick-sputter - Phase 1");
    Serial.println("Multi-Channel LED Blink Test");
    Serial.println("========================================");
    Serial.printf("Board: M5Stack Stamp Pico (ESP32 PICO32)\n");
    Serial.printf("Channel 0 (SK6812): GPIO %d\n", PIN_STATUS_SK6812);
    Serial.printf("Channel 1 (WS2811): GPIO %d (Red)\n", PIN_LED_CH1);
    Serial.printf("Channel 2 (WS2811): GPIO %d (Green)\n", PIN_LED_CH2);
    Serial.printf("Channel 3 (WS2811): GPIO %d (Blue)\n", PIN_LED_CH3);
    Serial.printf("Channel 4 (WS2811): GPIO %d (White)\n", PIN_LED_CH4);
    Serial.printf("Status LED: GPIO %d\n", PIN_STATUS_LED);
    Serial.printf("Button: GPIO %d\n", PIN_BUTTON);
    Serial.println("========================================\n");

    // Initialize button input (GPIO39 is input-only, has internal pullup)
    pinMode(PIN_BUTTON, INPUT);

    // Initialize status LED
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    // Initialize FastLED for all channels
    FastLED.addLeds<SK6812, PIN_STATUS_SK6812, GRB>(ledChannel0, 1);
    FastLED.addLeds<WS2811, PIN_LED_CH1, GRB>(ledChannel1, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH2, GRB>(ledChannel2, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH3, GRB>(ledChannel3, NUM_LEDS_PER_CHANNEL);
    FastLED.addLeds<WS2811, PIN_LED_CH4, GRB>(ledChannel4, NUM_LEDS_PER_CHANNEL);

    // Set brightness (25% for safe testing)
    FastLED.setBrightness(64);

    // Initialize all LEDs to off
    fill_solid(ledChannel0, 1, CRGB::Black);
    fill_solid(ledChannel1, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel2, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel3, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel4, NUM_LEDS_PER_CHANNEL, CRGB::Black);

    // Set channel 0 to initial color (red)
    ledChannel0[0] = colors[currentColorIndex];

    FastLED.show();

    Serial.println("Initialization complete.");
    Serial.println("Press button (GPIO39) to cycle channel 0 color and toggle status LED.\n");
}

void loop() {
    unsigned long currentTime = millis();

    // Handle button press with debouncing
    bool buttonState = digitalRead(PIN_BUTTON);
    if (buttonState == LOW && lastButtonState == HIGH) {
        // Button pressed (active low)
        if (currentTime - lastButtonPress > DEBOUNCE_MS) {
            lastButtonPress = currentTime;

            // Cycle channel 0 color
            currentColorIndex = (currentColorIndex + 1) % 4;
            ledChannel0[0] = colors[currentColorIndex];

            // Toggle status LED
            statusLedState = !statusLedState;
            digitalWrite(PIN_STATUS_LED, statusLedState ? HIGH : LOW);

            // Print status
            const char* colorNames[] = {"Red", "Green", "Blue", "White"};
            Serial.printf("Button pressed! Channel 0: %s, Status LED: %s\n",
                         colorNames[currentColorIndex],
                         statusLedState ? "ON" : "OFF");

            FastLED.show();
        }
    }
    lastButtonState = buttonState;

    // Blink channels 1-4 (first pixel only)
    if (currentTime - lastBlinkTime >= BLINK_INTERVAL_MS) {
        lastBlinkTime = currentTime;
        blinkState = !blinkState;

        if (blinkState) {
            // Turn on first pixel of each channel
            ledChannel1[0] = CRGB::Red;
            ledChannel2[0] = CRGB::Green;
            ledChannel3[0] = CRGB::Blue;
            ledChannel4[0] = CRGB::White;
            Serial.println("Channels 1-4: ON (R/G/B/W)");
        } else {
            // Turn off first pixel of each channel
            ledChannel1[0] = CRGB::Black;
            ledChannel2[0] = CRGB::Black;
            ledChannel3[0] = CRGB::Black;
            ledChannel4[0] = CRGB::Black;
            Serial.println("Channels 1-4: OFF");
        }

        FastLED.show();
    }
}
