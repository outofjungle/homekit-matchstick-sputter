#include <Arduino.h>
#include <FastLED.h>
#include "HomeSpan.h"
#include "config.h"
#include "led_channel.h"
#include "wifi_credentials.h"
#include "notification_pattern.h"

// LED Arrays for all channels
CRGB ledChannel1[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 26
CRGB ledChannel2[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 18
CRGB ledChannel3[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 25
CRGB ledChannel4[NUM_LEDS_PER_CHANNEL];        // WS2811 on GPIO 19

// Notification manager for visual feedback
NotificationManager* notificationMgr = nullptr;

// Button state machine for factory reset
enum ButtonState {
    BTN_IDLE,               // Not pressed
    BTN_PRESSED,            // Pressed < 5s (short press for mode cycle)
    BTN_NOTIFICATION,       // Showing warning animation (3x cycles) - runs to completion
    BTN_RESET,              // Factory reset triggered
    BTN_CANCELLED_CONFIRM   // Animation complete, button was released, showing green
};

ButtonState buttonState = BTN_IDLE;
unsigned long buttonPressStartMs = 0;
unsigned long confirmStartMs = 0;
bool buttonLastState = HIGH;  // GPIO39 is pulled high, LOW when pressed
bool buttonReleasedDuringAnimation = false;  // Track if button was released during 3x sequence
uint8_t currentDisplayMode = 0;  // For display mode cycling

// Handle short press (display mode cycling)
void handleShortPress() {
    currentDisplayMode = (currentDisplayMode + 1) % 4;  // Cycle through 4 modes
    Serial.printf("Display mode: %d\n", currentDisplayMode);
    // TODO: Implement actual display mode logic (placeholder for beads-4vz)
}

// Handle factory reset trigger
void handleFactoryReset() {
    Serial.println("FACTORY RESET TRIGGERED!");
    Serial.println("Erasing NVS and rebooting...");

    // Perform factory reset using HomeSpan command
    homeSpan.processSerialCommand("F");

    // Device will reboot after this
}

// Blank all LEDs in preparation for notification
void blankAllLEDs() {
    fill_solid(ledChannel1, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel2, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel3, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    fill_solid(ledChannel4, NUM_LEDS_PER_CHANNEL, CRGB::Black);
    FastLED.show();
}

// Update button state machine
void updateButtonStateMachine() {
    bool currentButtonState = digitalRead(PIN_BUTTON);
    unsigned long now = millis();

    // Debounce
    static unsigned long lastDebounceTime = 0;
    if ((now - lastDebounceTime) < DEBOUNCE_MS) {
        return;
    }

    // Detect button press/release edges
    bool buttonPressed = (currentButtonState == LOW);
    bool buttonJustPressed = (buttonPressed && buttonLastState == HIGH);
    bool buttonJustReleased = (!buttonPressed && buttonLastState == LOW);

    if (buttonJustPressed || buttonJustReleased) {
        lastDebounceTime = now;
    }

    buttonLastState = currentButtonState;

    // State machine logic
    switch (buttonState) {
        case BTN_IDLE:
            if (buttonJustPressed) {
                buttonState = BTN_PRESSED;
                buttonPressStartMs = now;
                Serial.println("Button pressed");
            }
            break;

        case BTN_PRESSED:
            if (buttonJustReleased) {
                // Short press - cycle display mode
                unsigned long pressDuration = now - buttonPressStartMs;
                if (pressDuration < FACTORY_RESET_WARNING_MS) {
                    handleShortPress();
                }
                buttonState = BTN_IDLE;
            } else if ((now - buttonPressStartMs) >= FACTORY_RESET_WARNING_MS) {
                // Held for 5s - enter notification state
                buttonState = BTN_NOTIFICATION;
                buttonReleasedDuringAnimation = false;  // Reset flag
                Serial.println("Entering factory reset warning mode...");

                // Blank ALL LEDs before starting animation
                blankAllLEDs();

                // Start warning animation (3 complete cycles)
                // ~300ms per step = ~2.4s per cycle, ~7.2s total for 3 cycles
                notificationMgr->start(PATTERN_WARNING, CRGB::Blue, 300, 3);
            }
            break;

        case BTN_NOTIFICATION:
            // Track button release but let animation complete
            if (buttonJustReleased) {
                Serial.println("Button released - animation will complete, then show cancellation");
                buttonReleasedDuringAnimation = true;
            }

            // Check if animation completed (3 cycles done)
            if (notificationMgr->getCycleCount() >= 3) {
                notificationMgr->stop();

                if (buttonReleasedDuringAnimation || !buttonPressed) {
                    // Button was released during animation - show green confirmation
                    Serial.println("Animation complete - reset cancelled (button was released)");
                    buttonState = BTN_CANCELLED_CONFIRM;
                    confirmStartMs = now;
                    buttonReleasedDuringAnimation = false;

                    // Show green confirmation
                    notificationMgr->start(PATTERN_SOLID, CRGB::Green, 0, 0);
                } else {
                    // Button still held - initiate factory reset
                    Serial.println("Animation complete - button still held, initiating reset");
                    buttonState = BTN_RESET;
                    handleFactoryReset();
                }
            }
            break;

        case BTN_CANCELLED_CONFIRM:
            if ((now - confirmStartMs) >= FACTORY_RESET_CONFIRM_MS) {
                // 3 seconds elapsed - restore previous state and resume
                Serial.println("Resuming normal operation");
                notificationMgr->stop();
                buttonState = BTN_IDLE;
            }
            break;

        case BTN_RESET:
            // Factory reset in progress, device will reboot
            break;
    }
}

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

    // Initialize notification manager
    notificationMgr = new NotificationManager(ledChannel1, ledChannel2, ledChannel3, ledChannel4);
    Serial.println("Notification manager initialized.");

    // Initialize button pin
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    Serial.println("Button pin configured (GPIO39).");

    // Initialize status LED pin
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);  // Start off during setup
    Serial.println("Status LED pin configured (GPIO22).");

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

    // Turn on status LED to indicate device is active
    digitalWrite(PIN_STATUS_LED, HIGH);
    Serial.println("Status LED ON - device active");
}

void loop() {
    // Update button state machine for factory reset
    updateButtonStateMachine();

    // Update notification animations if active
    if (notificationMgr->isActive()) {
        bool stillRunning = notificationMgr->update(NUM_LEDS_PER_CHANNEL);
        // Animation completion is now handled in the state machine
        // (checking getCycleCount() in BTN_NOTIFICATION state)
    }

    // Poll HomeSpan for HomeKit events
    homeSpan.poll();

    // Update LED strips (called after every HomeSpan update)
    FastLED.show();
}
